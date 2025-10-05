#pragma once

u32 LoadTga_move(const c8* Path, const c8* UniqueName)
{
  obj_bitmap* ObjBitmap = LoadTGA(TransientAllocator, Path);
  if(UniqueName == 0 || *UniqueName =='\0')
  {
    UniqueName = Path;
  }
  u32 TextureHandle = CopyObjBitmapToTexture(UniqueName, ObjBitmap);
  return TextureHandle; 
}

obj_bitmap* LoadTGA(void* (*AssetAllocator)(u32 ByteSize), const char* FileName)
{
  debug_read_file_result ReadResult = Platform.DEBUGPlatformReadEntireFile(FileName);

  if( !ReadResult.ContentSize ) { return {}; }


  u32 HeaderSize = sizeof(tga_header);
  Assert(ReadResult.ContentSize > HeaderSize);

  tga_header& Header = *(tga_header*) ReadResult.Contents;

  // Note Jakob: Seems like if IDLength > 0 it specifies the length of the
  //             image ID Field which comes after the header.
  //             But specification says Header.IDLength == 1 means it
  //             just exists. So I will treat the field like a length between
  //             header and pixel data unless it's 1 and I should investigate.
  Assert(Header.IDLength !=  1);
  Assert(Header.XOrigin == 0 );
  Assert(Header.YOrigin == 0 );

  u32 BytesPerPixel = Header.PixelDepth/8;

  obj_bitmap* Result = (obj_bitmap*) AssetAllocator( sizeof(obj_bitmap) );
  Result->Pixels = AssetAllocator((Header.Width*Header.Height) * sizeof(u32));
  Result->Width = Header.Width;
  Result->Height = Header.Height;
  Result->BPP = 32;

  Result->PathLength = jstr::StringLength( FileName );
  Result->Path = (char*) AssetAllocator((Result->PathLength+1) * sizeof(char));
  jstr::CopyStrings( Result->PathLength, FileName, Result->PathLength+1, Result->Path );

  char* OnePastLastSlash = jstr::FindLastOf( "\\", FileName)+1;
  Result->NameLength = jstr::StringLength( OnePastLastSlash );
  Result->Name = (char*) AssetAllocator((Result->NameLength+1) * sizeof(char));
  jstr::CopyStrings( Result->NameLength, OnePastLastSlash, Result->NameLength+1, Result->Name );
  

  Result->Pixels = AssetAllocator((Header.Width*Header.Height) * sizeof(u32));
  u8 ImageIdentificationFieldSize = Header.IDLength == 1 ? 0 : Header.IDLength;
  u32 PixelCount = Header.Width*Header.Height;
  u32* DstPxl = (u32*) Result->Pixels;
  u8*  SrcData = ((u8*) ReadResult.Contents) + (sizeof(tga_header) + ImageIdentificationFieldSize);

  switch(Header.ImageType)
  {
    case TGA_UNCOMPRESSED_COLOR_MAPPED: INVALID_CODE_PATH
    case TGA_UNCOMPRESSED_RGB:
    case TGA_UNCOMPRESSED_BLACK_AND_WHITE:
    {
      // Uncompressed true color image
      u32 MinimumFileSize = (HeaderSize + PixelCount * BytesPerPixel );
      Assert(ReadResult.ContentSize >= MinimumFileSize);

      for( u32 i = 0; i < PixelCount; ++i )
      {
        *DstPxl++ = ReadPixel(BytesPerPixel, SrcData);
        SrcData += BytesPerPixel;
      }
    }break;
    case TGA_RLE_COLOR_MAPPED:
    {
      // Header.ColorMapType is always 1 for Color Mapped images
      Assert(Header.ColorMapType == 1);
      u32 ColorMapEntrySizeByte = (Header.ColorMapEntrySize / 8);
      u32 ColorDataOffset = ColorMapEntrySizeByte * Header.ColorMapLength;
      u8* ColorMapData = SrcData;
      SrcData += ColorDataOffset;
      
      u32 PixelsRead = 0;
      while(PixelsRead < PixelCount)
      {
        u8 RunLengthPaket = *SrcData++;

        b32 IsRunLenghtPacket = (RunLengthPaket >> 7);
        u32 RunLength = (RunLengthPaket & 0x7F)+1;
        if(IsRunLenghtPacket)
        {
          // Run Length Packet: Read One pixel and increment Src Data Once
          u32 ColorIndex = ReadColorIndex(BytesPerPixel, SrcData);
          SrcData += BytesPerPixel;
          u32 PixelData = ReadColorMapPixel(ColorIndex, ColorMapEntrySizeByte, ColorMapData);
          for(u32 i = 0; i < RunLength; ++i)
          {
            *DstPxl++ = PixelData;
          }
        }else{
          // Raw Packets: Read NrRunLength from SrcData Sequentially
          for( u32 i = 0; i < RunLength; ++i )
          {
            u32 ColorIndex = ReadColorIndex(BytesPerPixel, SrcData);
            u32 PixelData = ReadColorMapPixel(ColorIndex, ColorMapEntrySizeByte, ColorMapData);
            *DstPxl++ = PixelData;
            SrcData += BytesPerPixel;
          }
        }
        PixelsRead += RunLength;
      }

    }break;
    case TGA_RLE_RGB:
    case TGA_COMPRESSED_BLACK_AND_WHITE:
    {
      // Run-Length Endoded True color Image
      ReadRunLengthEncodedRGB(PixelCount, BytesPerPixel, (u32*) DstPxl, SrcData);
    }break;
    case TGA_COMPRESSED_COLOR_MAPPED_A:
    case TGA_COMPRESSED_COLOR_MAPPED_B:
    default: INVALID_CODE_PATH
  }

  Platform.DEBUGPlatformFreeFileMemory(ReadResult.Contents);

  return Result;
}

