#include "io/obj.h"
#include "math/vector_math.h"
#include "commons/string.h"
#include "commons/macros.h"
#include "containers/data_containers.h"


v4 ParseNumbers(char* String)
{
  char* Start = jstr::FindFirstNotOf( " \t", String );
  char WordBuffer[STR_MAX_WORD_LENGTH];
  s32 CoordinateIdx = 0;
  v4 Result = V4(0,0,0,1);
  while( Start )
  {
    Assert(CoordinateIdx < 4);

    char* End = jstr::FindFirstOf( " \t", Start);

    size_t WordLength = ( End ) ? (End - Start) : jstr::StringLength(Start);

    Assert(WordLength < STR_MAX_WORD_LENGTH);

    utils::Copy( WordLength, Start, WordBuffer );
    WordBuffer[WordLength] = '\0';

    r64 value = jstr::StringToReal64(WordBuffer);
    Result.E[CoordinateIdx++] = (r32) value;

    Start = (End) ? jstr::FindFirstNotOf(" \t", End) : End;
  }

  return Result;
}

struct obj_start_string
{
  u32 Enum;
  u32 StringLength;
  char String[16];
};

struct type_string_pair
{
  char* String;
  u32 Enum;
};

// Description http://www.paulbourke.net/dataformats/mtl/

// d_factor says how a material disolves into the background.
// Unlike transparency it's not dependant upon material thickness.

// dissolve = 1.0 - (N*v)(1.0-factor)
enum d_factor_types
{
  D_FACTOR_EMPTY,
  D_FACTOR_HALO   // Halo means Materials disolve is dependant upon the orientation of the viewer. Like a sphere is more transparent in the middle than at the edges
};


enum mtl_data_types
{
  MTL_EMPTY,
  // Material name statement:
  MTL_NEW_MATERIAL,   // (newmtl) my_mtl

  // Material color and illumination statements:
  MTL_KA,       //  Ambient:   (Ka) 0.0435 0.0435 0.0435  
  MTL_KD,       //  Diffuse:   (Kd)0.1086 0.1086 0.1086   
  MTL_KS,       //  Specular:  (Ks) 0.0000 0.0000 0.0000  
  MTL_KE,       //  Emmissive: (Ke) 0.0000 0.0000 0.0000  
  MTL_TF,       // (Tf) 0.9885 0.9885 0.9885  
  MTL_ILLUMINATION,   // (illum) 6
  MTL_D,        // (d) -halo 0.6600
  MTL_NS,       // (Ns) 10.0000
  MTL_SHARPNESS,    // (sharpness) 60
  MTL_NI,       // (Ni) 1.19713

  // Texture map statements:
  MTL_MAP_KA,     // (map_Ka)   -s 1 1 1 -o 0 0 0 -mm 0 1 chrome.mpc
  MTL_MAP_KD,     // (map_Kd)   -s 1 1 1 -o 0 0 0 -mm 0 1 chrome.mpc
  MTL_MAP_KS,     // (map_Ks)   -s 1 1 1 -o 0 0 0 -mm 0 1 chrome.mpc
  MTL_MAP_NS,     // (map_Tf)   -s 1 1 1 -o 0 0 0 -mm 0 1 wisp.mps
  MTL_MAP_BUMP,   // (map_Bump) -s 1 1 1 -o 0 0 0 -bm 1 sand.mpb
  MTL_MAP_D,      // (map_d)    -s 1 1 1 -o 0 0 0 -mm 0 1 wisp.mps
  MTL_DISP,       // (disp)     -s 1 1 .5 wisp.mps
  MTL_DECAL,      // (decal)    -s 1 1 1 -o 0 0 0 -mm 0 1 sand.mps

  // Reflection map statement:
  MTL_REFLECTION_MAP  // refl -type sphere -mm 0 1 clouds.mpc
};

type_string_pair GetMTLDataType( u32 StringLength, char* String )
{
  local_persist obj_start_string TypeMap[24] =
  {
    { MTL_EMPTY,           1, "#",        },
    { MTL_EMPTY,           2, "\r\n"      },
    { MTL_EMPTY,           1, "\n"        },
    { MTL_NEW_MATERIAL,    7, "newmtl "   },
    { MTL_KA,              3, "Ka "       },    // Ambient Coefficient
    { MTL_KD,              3, "Kd "       },    // Diffuse Coefficient
    { MTL_KS,              3, "Ks "       },    // Specular Coefficient
    { MTL_KE,              3, "Ke "       },    // Emmissive Coefficient
    { MTL_TF,              3, "Tf "       },
    { MTL_ILLUMINATION,    6, "illum "    },
    { MTL_D,               2, "d "        },
    { MTL_NS,              3, "Ns "       },
    { MTL_SHARPNESS,      10, "sharpness "},
    { MTL_NI,              3, "Ni "       },
    { MTL_MAP_KA,          7, "map_Ka "   },
    { MTL_MAP_KD,          7, "map_Kd "   },
    { MTL_MAP_KS,          7, "map_Ks "   },
    { MTL_MAP_NS,          7, "map_Ns "   },
    { MTL_MAP_BUMP,        9, "map_Bump " },
    { MTL_MAP_BUMP,        5, "bump "     },
    { MTL_MAP_D,           6, "map_d "    },
    { MTL_DISP,            5, "disp "     },
    { MTL_DECAL,           6, "decal "    },
    { MTL_REFLECTION_MAP,  5, "refl "     }
    };

  type_string_pair Result = {};
  for( u32 TypeIDX = 0; TypeIDX < ArrayCount(TypeMap); ++TypeIDX )
  {
    obj_start_string& Type = TypeMap[TypeIDX];
    if(jstr::BeginsWith(Type.StringLength, Type.String, StringLength, String ))
    {
        Result.Enum = Type.Enum;
        Result.String = jstr::FindFirstNotOf( " \t", String+Type.StringLength);
        return Result;
    }
  }

  Assert(0);
  return {};
}



enum obj_data_types
{
  // Misc
  OBJ_EMPTY,

  // Vertex data
  OBJ_GEOMETRIC_VERTICES,     // (v)    1
  OBJ_TEXTURE_VERTICES,       // (vt)
  OBJ_VERTEX_NORMALS,       // (vn)
  OBJ_PARAMETER_SPACE_VERTICES,   // (vp)

  // Free-form curve/surface attributes
  OBJ_CURVE_OR_SURFACE_TYPE,  // (cstype)   5
  OBJ_DEGREE,                 // (deg)
  OBJ_BASIS_MATRIX,           // (bmat)
  OBJ_STEP_SIZE,              // (step)

  // Elements
  OBJ_POINT,    // (p)            9
  OBJ_LINE,     // (l)
  OBJ_FACE,     // (f)
  OBJ_CURVE,    // (curv)
  OBJ_2D_CURVE,   // (curv2)
  OBJ_SURFACE,  // (surf)

  // Free-form curve/surface body statements
  OBJ_PARAMETER_VALUES,     // (parm)   15
  OBJ_OUTER_TRIMMING_LOOP,  // (trim)
  OBJ_INNER_TRIMMING_LOOP,  // (hole)
  OBJ_SPECIAL_CURVE,      // (scrv)
  OBJ_SPECIAL_POINT,      // (sp)
  OBJ_END_STATEMENT,      // (end)

  // Connectivity between free-form surfaces
  OBJ_CONNECT,      // (con)      21

  // Grouping
  OBJ_GROUP_NAME,     // (g)
  OBJ_SMOOTHING_GROUP,  // (s)
  OBJ_MERGING_GROUP,    // (mg)
  OBJ_OBJECT_NAME,    // (o)        25

  // Display/render attributes
  OBJ_BEVEL_INTERPOLATION,        // (bevel)
  OBJ_COLOR_INTERPOLATION ,         // (c_interp)
  OBJ_DISSOLVE_INTERPOLATION,       // (d_interp)
  OBJ_LEVEL_OF_DETAIL,          // (lod)
  OBJ_MATERIAL_NAME,            // (usemtl)
  OBJ_MATERIAL_LIBRARY,           // (mtllib)
  OBJ_SHADOW_CASTING,           // (shadow_obj)
  OBJ_RAY_TRACING,            // (trace_obj)
  OBJ_CURVE_APPROXIMATION_TECHNIQUE,    // (ctech)
  OBJ_SURFACE_APPROXIMATION_TECHNIQUE,  // (stech)
};


type_string_pair GetOBJDataType( u32 StringLength, char* String )
{
  local_persist obj_start_string TypeMap[38] =
  {
    { OBJ_EMPTY,                              1 , "#",          },
    { OBJ_EMPTY,                              2 , "\r\n"        },
    { OBJ_EMPTY,                              1 , "\n"          },
    { OBJ_GEOMETRIC_VERTICES,                 2 , "v "          },
    { OBJ_TEXTURE_VERTICES,                   3 , "vt "         },
    { OBJ_VERTEX_NORMALS,                     3 , "vn "         },
    { OBJ_PARAMETER_SPACE_VERTICES,           3 , "vp "         },
    { OBJ_CURVE_OR_SURFACE_TYPE,              7 , "cstype "     },
    { OBJ_DEGREE,                             4 , "deg "        },
    { OBJ_BASIS_MATRIX,                       5 , "bmat "       },
    { OBJ_STEP_SIZE,                          5 , "step "       },
    { OBJ_POINT,                              2 , "p "          },
    { OBJ_LINE,                               2 , "l "          },
    { OBJ_FACE,                               2 , "f "          },
    { OBJ_CURVE,                              5 , "curv "       },
    { OBJ_2D_CURVE,                           6 , "curv2 "      },
    { OBJ_SURFACE,                            5 , "surf "       },
    { OBJ_PARAMETER_VALUES,                   5 , "parm "       },
    { OBJ_OUTER_TRIMMING_LOOP,                5 , "trim "       },
    { OBJ_INNER_TRIMMING_LOOP,                5 , "hole "       },
    { OBJ_SPECIAL_CURVE,                      5 , "scrv "       },
    { OBJ_SPECIAL_POINT,                      3 , "sp "         },
    { OBJ_END_STATEMENT,                      3 , "end"         },
    { OBJ_CONNECT,                            4 , "con "        },
    { OBJ_GROUP_NAME,                         2 , "g "          },
    { OBJ_SMOOTHING_GROUP,                    2 , "s "          },
    { OBJ_MERGING_GROUP,                      3 , "mg "         },
    { OBJ_OBJECT_NAME,                        2 , "o "          },
    { OBJ_BEVEL_INTERPOLATION,                6 , "bevel "      },
    { OBJ_COLOR_INTERPOLATION,                9 , "c_interp "   },
    { OBJ_DISSOLVE_INTERPOLATION,             9 , "d_interp "   },
    { OBJ_LEVEL_OF_DETAIL,                    4 , "lod "        },
    { OBJ_MATERIAL_NAME,                      7 , "usemtl "     },
    { OBJ_MATERIAL_LIBRARY,                   7 , "mtllib "     },
    { OBJ_SHADOW_CASTING,                     11, "shadow_obj " },
    { OBJ_RAY_TRACING,                        10, "trace_obj "  },
    { OBJ_CURVE_APPROXIMATION_TECHNIQUE,      6,  "ctech "      },
    { OBJ_SURFACE_APPROXIMATION_TECHNIQUE,    6,  "stech "      }
  };

  type_string_pair Result = {};
  for( u32 TypeIDX = 0; TypeIDX < ArrayCount(TypeMap); ++TypeIDX )
  {
    obj_start_string& Type = TypeMap[TypeIDX];
    if(jstr::BeginsWith(Type.StringLength, Type.String, StringLength, String ))
    {
        Result.Enum = Type.Enum;
        Result.String = jstr::FindFirstNotOf( " \t", String+Type.StringLength);
        return Result;
    }
  }


  Assert(0);
  return {};
}


bool GetTrimmedLine( char* SrcLineStart, char* SrcLineEnd, midx* DstLength, char* DstLine )
{
  char* TrimEnd = SrcLineEnd-1;
  while( (TrimEnd >= SrcLineStart) && ( (*TrimEnd == '\n') || (*TrimEnd == '\r') ) )
  {
    --TrimEnd;
  }
  ++TrimEnd;

  char* TrimStart = SrcLineStart;
  while( (TrimStart < TrimEnd) && ((*TrimStart == ' ') || (*TrimStart == '\t')) )
  {
    ++TrimStart;
  }

  SrcLineStart = TrimStart;

  midx Length = TrimEnd - SrcLineStart;

  Assert(Length < STR_MAX_LINE_LENGTH);

  if(Length < 2)
  {
    return false;
  }

  utils::Copy( Length, SrcLineStart, DstLine );
  DstLine[Length] = '\0';
  *DstLength = Length;
  return true;
}

void TriangulateLine(fifo_queue<u32>* Queue, fifo_queue<u32>* ResultQueue)
{
  // Triangulate:
  // We can assume the original .obj file has right handed
  // orientation to their faecs.
  //  4--3             3      4--3
  //  |  | becomes    /|  and | /
  //  |  |           / |      |/
  //  1--2          1--2      1
  //  Make sure they're righthanded
  if(Queue->GetSize() <= 3)
  {
    while(!Queue->IsEmpty())
    {
      ResultQueue->Push(Queue->Pop());
    }
    return;
  }

  // Here we assume the Queue Size is greater than three
  u32 Triangle[3] = {};
  Triangle[0] = Queue->Pop();
  Triangle[1] = Queue->Pop();
  Triangle[2] = Queue->Pop();

  while(true)
  {
    ResultQueue->Push(Triangle[0]);
    ResultQueue->Push(Triangle[1]);
    ResultQueue->Push(Triangle[2]);
    if(!Queue->IsEmpty())
    {
      Triangle[1] = Triangle[2];
      Triangle[2] = Queue->Pop();
    }else{
      return;
    }
  }
}

struct group_to_parse
{
  u32 GroupNameLength;
  char* GroupName;

  u32* SmoothingGroup;

  fifo_queue<u32> vi;
  fifo_queue<u32> ti;
  fifo_queue<u32> ni;

  u32 MaterialNameLength;
  char* MaterialName;
  u32 IndiceOffset;
};

void ParseFaceLine(memory_arena* Arena, char* ParsedLine, group_to_parse* ActiveGroup )
{
  u32 NrVerticesInFace = jstr::GetWordCount( ParsedLine );
  Assert( NrVerticesInFace >= 3);

  fifo_queue<u32> vi = fifo_queue<u32>(Arena);
  fifo_queue<u32> ti = fifo_queue<u32>(Arena);
  fifo_queue<u32> ni = fifo_queue<u32>(Arena);

  char* Start = ParsedLine;
  char WordBuffer[STR_MAX_WORD_LENGTH];
  while( Start )
  {
    char* End = jstr::FindFirstOf( " \t", Start);

    size_t WordLength = ( End ) ? (End - Start) : jstr::StringLength(Start);

    Assert(WordLength < STR_MAX_WORD_LENGTH);

    utils::Copy( WordLength, Start, WordBuffer );
    WordBuffer[WordLength] = '\0';

    char* StartNr = WordBuffer;
    char* EndNr   = 0;
    u32 IndexType = 0;

    while(StartNr)
    {
      EndNr   = jstr::FindFirstOf("/", StartNr);
      if(EndNr)
      {
        *EndNr++ = '\0';
      }

      u32 Number = (u32) jstr::StringToReal64(StartNr)-1;
      Assert(Number>=0);
      switch(IndexType)
      {
        // Vertex Index
        case 0:
        {
          vi.Push(Number);
        }break;
        // Texture Index
        case 1:
        {
          ti.Push(Number);
        }break;
        // Normal Index
        case 2:
        {
          ni.Push(Number);
        }break;

        default:
        {
          INVALID_CODE_PATH
        }break;
      }

      // A face specified with ____ looks like: "_____"
      //   1: Vertice, Texture Vertice and Normals: "10/11/12"
      //   2: Only Vertice and Normals:             "10//12"
      //   3: Only Vertice and Texture Vertice:     "10/11"
      //   4: Only Vertice                          "10"
      // Case 1, 3 and 4 is handled by just increasing i sequentially.
      ++IndexType;

      // Case 2 requires us to increase i two steps skipping case IndexType==1.
      if( EndNr && *EndNr == '/' )
      {
        ++IndexType;
      }

      StartNr = jstr::FindFirstNotOf("/", EndNr);

    }

    Start = (End) ? jstr::FindFirstNotOf(" \t", End) : End;
  }

  // Make sure we have vertex, texture and normal indeces of the right size.
  Assert( vi.GetSize() == NrVerticesInFace);
  Assert((ti.GetSize() == NrVerticesInFace) || (ti.GetSize() == 0) );
  Assert((ni.GetSize() == NrVerticesInFace) || (ni.GetSize() == 0) );

  TriangulateLine(&vi, &ActiveGroup->vi);

  if(!ti.IsEmpty())
  {
    TriangulateLine(&ti, &ActiveGroup->ti);
  }

  if(!ni.IsEmpty())
  {
    TriangulateLine(&ni, &ActiveGroup->ni);
  }
}


// http://www.paulbourke.net/dataformats/tga/
enum TGA_DATA_TYPE {
  TGA_NO_DATA = 0,                      // No image data included.
  TGA_UNCOMPRESSED_COLOR_MAPPED = 1,    // Uncompressed, color-mapped images.
  TGA_UNCOMPRESSED_RGB = 2,             // Uncompressed, RGB images.
  TGA_UNCOMPRESSED_BLACK_AND_WHITE = 3, // Uncompressed, black and white images.
  TGA_RLE_COLOR_MAPPED = 9,             // Runlength encoded color-mapped images.
  TGA_RLE_RGB = 10,                     // Runlength encoded RGB images.
  TGA_COMPRESSED_BLACK_AND_WHITE = 11,  // Compressed, black and white images.
  TGA_COMPRESSED_COLOR_MAPPED_A = 32,   // Compressed color-mapped data, using Huffman, Delta, and
                                        // runlength encoding.
  TGA_COMPRESSED_COLOR_MAPPED_B = 33    // Compressed color-mapped data, using Huffman, Delta, and
                                        // runlength encoding. 4-pass quadtree-type process.
};

// makes the packing compact
#pragma pack(push, 1)
struct tga_header
{
  /*
   * ID length:
   * 0 if image file contains no color map
     * 1 if present
     * 2–127 reserved by Truevision
     * 128–255 available for developer use

     Note Jakob, Seems like if > 0 it specifies the length of the
             image ID Field which comes after the header.
     */
  u8 IDLength;

  /*
   * ColorMapType:
     * 0 if image file contains no color map
     * 1 if present
     * 2–127 reserved by Truevision
     * 128–255 available for developer use
     */
  u8 ColorMapType;

  /*
   * ImageType is enumerated in the lower three bits, with the fourth bit as a flag for RLE. Some possible values are:
     * 0 no image data is present
     * 1 uncompressed color-mapped image
     * 2 uncompressed true-color image
     * 3 uncompressed black-and-white (grayscale) image
     * 9 run-length encoded color-mapped image
     * 10 run-length encoded true-color image
     * 11 run-length encoded black-and-white (grayscale) image
     * 32 Compressed color-mapped data, using Huffman, Delta, and
          runlength encoding.
     * 33 Compressed color-mapped data, using Huffman, Delta, and
          runlength encoding.  4-pass quadtree-type process.
   */
  u8 ImageType;

  // ColorMapSpecification
  u16 ColorMapOrigin;    // index of first color map entry that is included in the file
  u16 ColorMapLength;    // number of entries of the color map that are included in the file
  u8  ColorMapEntrySize; // number of bits per pixel

  // ImageSpecification
  u16 XOrigin;      // absolute coordinate of lower-left corner for displays where origin is at the lower left
  u16 YOrigin;      // as for X-origin.  0,0 emans first pixel is lower left.
                          // Data is stored in BGRA Format.
  u16 Width;        // width in pixels
  u16 Height;       // height in pixels
  u8  PixelDepth;     // Bits per pixel
  u8  ImageDescriptor;  // bits 3-0 give the alpha channel depth, bits 5-4 give direction
};
#pragma pack(pop)

u32 ReadPixel(u32 BytesPerPixel, u8* SrcData)
{
  u8 R,G,B,A;
  R = G = B = A = 0;
  switch(BytesPerPixel)
  {
    case 1:
    {
      R = G = B = *SrcData;
      A = 255;
    }break;
    // RGB but No Alpha Channel
    case 3:
    {
      B = *(SrcData+0);
      G = *(SrcData+1);
      R = *(SrcData+2);
      A = 255;
    }break;
    // Full BGRA
    case 4:
    {
      // BGRA Format
      B = *(SrcData + 0);
      G = *(SrcData + 1);
      R = *(SrcData + 2);
      A = *(SrcData + 3);
    }break;
    default:
    {
      INVALID_CODE_PATH
    }break;
  }
  // BGRA Format
  //Pixel = (B << 24) | (G << 16) | (R << 8) | (A << 0);
  // ARGB Format
  u32 Pixel = (A << 24) | (R << 16) | (G << 8) | (B << 0);
  return Pixel;
}

void ReadRunLengthEncodedRGB( const u32 NrPixels, const u32 BytesPerPixel, u32* DstPxl, u8* SrcData )
{
  u32 PixelsRead = 0;
  while(PixelsRead < NrPixels)
  {
    u8 RunLengthPaket = *SrcData++;

    b32 IsRunLenghtPacket = (RunLengthPaket >> 7);
    u32 RunLength = (RunLengthPaket & 0x7F)+1;
    if(IsRunLenghtPacket)
    {
      // Run Length Packet: Read One pixel and increment Src Data Once
      u32 Pixel = ReadPixel(BytesPerPixel, SrcData);
      SrcData += BytesPerPixel;
      for(u32 i = 0; i < RunLength; ++i)
      {
        // And add that one pixel RunLength times to our bitmap
        *DstPxl++ = Pixel;
      }
    }else{
      // Raw Packets: Read NrRunLength from SrcData Sequentially
      for( u32 i = 0; i <RunLength; ++i )
      {
        *DstPxl++ = ReadPixel(BytesPerPixel, SrcData);
        SrcData += BytesPerPixel;
      }
    }
    PixelsRead += RunLength;
  }
}


u32 ReadColorIndex(u8 BytesPerPixel, u8* SrcData)
{
  u8 b0,b1,b2,b3;
  b0 = b1 = b2 = b3 = 0;

  switch(BytesPerPixel)
  {
    case 1:
    {
      b0 = SrcData[0];
    }break;
    case 2:
    {
      b0 = SrcData[0];
      b1 = SrcData[1];
    }break;
    case 3:
    {
      b0 = SrcData[0];
      b1 = SrcData[1];
      b2 = SrcData[2];
    }break;
    case 4:
    {
      b0 = SrcData[0];
      b1 = SrcData[1];
      b2 = SrcData[2];
      b3 = SrcData[3];
    }break;
    default:
    {
      INVALID_CODE_PATH
    }break;
  }
  u32 Index = (b3 << 24) | (b2 << 16) | (b1 << 8) | (b0 << 0);
  return Index;
}
u32 ReadColorMapPixel(u32 ColorIndex, u32 ColorMapEntrySizeByte, u8* ColorMapData)
{
  u8 R,G,B,A = 0;
  u32 ColorOffset = ColorIndex * ColorMapEntrySizeByte;
  switch(ColorMapEntrySizeByte)
  {
    case 2:
    {
      // Color stored as
      // ARRRRRGG GGGBBBBB ordered lo-hi
      // So:
      // Byte 0 = GGGBBBBB
      // Byte 1 = ARRRRRGG

      // TODO: Unverified code path
      u8 b0 = ColorMapData[ColorOffset + 0];
      u8 b1 = ColorMapData[ColorOffset + 1];
      B = b0 & 0x1F;
      G = ( 5 >> b0) | (b1 & 0x03);
      R = ( 2 >> b1 ) & 0x1F;
      A = 255;
    }break;
    case 3:
    {
      B = ColorMapData[ColorOffset + 0];
      G = ColorMapData[ColorOffset + 1];
      R = ColorMapData[ColorOffset + 2];
      A = 255;
    }break;
    case 4:
    {
      // TODO: Unverified code path
      B = ColorMapData[ColorOffset + 0];
      G = ColorMapData[ColorOffset + 1];
      R = ColorMapData[ColorOffset + 2];
      A = ColorMapData[ColorOffset + 3];
    }break;
    default:
    {
      INVALID_CODE_PATH
    }break;
  }

  u32 Pixel = (A << 24) | (R << 16) | (G << 8) | (B << 0);
  return Pixel;
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



fifo_queue<char*> ExtractMTLSettings(memory_arena* Arena, u32 ArgumentCount, const char* Setting, const char* ArgumentSeparationTokens, char* SourceString, char* LeftOverString )
{
  u32 SettingLength = jstr::StringLength(Setting);
  u32 SourceStringLength = jstr::StringLength(SourceString);
  char* SettingStartPtr = 0;
  if( ( SettingStartPtr = jstr::Contains( SettingLength, Setting, SourceStringLength, SourceString )) == 0 ){ return {}; }

  char* SettingPtr = SettingStartPtr + SettingLength;

  fifo_queue<char*> Result = fifo_queue<char*>(Arena);

  for(u32 i = 0; i < ArgumentCount; ++i)
  {
    // Move to argument
    char* Start = jstr::FindFirstNotOf( ArgumentSeparationTokens, SettingPtr );

    // Should be pointing to argument
    Assert(Start);

    // Move to after the argument
    char* End = jstr::FindFirstOf( ArgumentSeparationTokens, Start );
    End = ( End ) ?  End : (SourceString + SourceStringLength);

    u64 Length = End - Start;
    char* PushSetting = (char*) PushArray(Arena, Length+1, char);
    jstr::CopyStrings( Length, Start, Length+1, PushSetting );
    Result.Push(PushSetting);

    SettingPtr = End;
  }

  u64 LengthBefore = SettingStartPtr - SourceString;
  u64 LengthBetween = SettingPtr - SettingStartPtr;
  Assert(SourceStringLength >= (LengthBefore + LengthBetween) );
  u64 LengthAfter = SourceStringLength - (LengthBefore + LengthBetween);
  jstr::CatStrings( LengthBefore, SourceString,
      LengthAfter, SettingPtr, LeftOverString );

  return Result;
}

file_local void CreateNewFilePath(const char* BaseFilePath, const char* NewFileName, char* NewFilePath )
{
  char* Pos = jstr::FindLastOf( "\\", BaseFilePath );
  Assert( Pos )
  ++Pos;
  u32 BaseFolderLength = jstr::StringLength(BaseFilePath)-jstr::StringLength(Pos);


  char* Start = jstr::FindFirstNotOf( " \t", NewFileName);
  char* End = jstr::FindLastNotOf(" \t" , Start);
  ++End;
  Assert(Start < End)

  u32 NewFileNameLength =(u32) (End - Start);
  Assert( (BaseFolderLength + NewFileNameLength) < STR_MAX_LINE_LENGTH );

  jstr::CatStrings(  BaseFolderLength,    BaseFilePath,
            NewFileNameLength,   Start, NewFilePath );
}

c8* GetNextLine(c8* Src, c8* End, midx* DstLen, c8* Dst){

  char LineBuffer[STR_MAX_LINE_LENGTH] = {};
  c8* Buf = LineBuffer;
  c8* StartLine = Src;
  c8* EndLine = jstr::FindFirstOf("\n\\", StartLine);
  midx CharsCopied = 0;
  while(EndLine && *EndLine == '\\')
  {
    midx CharsToCopy = EndLine - StartLine;
    utils::Copy(CharsToCopy, StartLine, Buf);
    Buf += CharsToCopy;
    CharsCopied += CharsToCopy;

    StartLine = jstr::FindFirstOf("\n", EndLine+1) + 1;
    EndLine   = jstr::FindFirstOf("\n\\", StartLine);
  }
  Assert(*EndLine == '\n' || *EndLine == '\0');

  EndLine = EndLine ? EndLine+1 : End;

  midx CharsToCopy = EndLine - StartLine;
  utils::Copy(CharsToCopy, StartLine, Buf);
  Buf += CharsToCopy;
  CharsCopied += CharsToCopy;
  *Buf = '\0';

  if(!GetTrimmedLine( LineBuffer, LineBuffer + CharsCopied, DstLen, Dst))
  {
    *DstLen = 0;
  }

  return EndLine;
}

file_local void AssertNoMapSettings(c8* String)
{
  u32 SettingLength = jstr::StringLength( String );

  // Unimplemented Settings
  // -blendu on | off
  Assert( ! jstr::Contains( 8, "-blendu ", SettingLength, String ) );

  // -blendv on | off
  Assert( ! jstr::Contains( 8, "-blendv ", SettingLength, String ) );

  // -cc on | off
  Assert( ! jstr::Contains( 4, "-cc ",     SettingLength, String ) );

  // -clamp on | off
  Assert( ! jstr::Contains( 7, "-clamp ",  SettingLength, String ) );

  // -mm base gain
  Assert( ! jstr::Contains( 4, "-mm ",     SettingLength, String ) );

  // -o u v w
  Assert( ! jstr::Contains( 3, "-o ",      SettingLength, String ) );

  // -s u v w
  Assert( ! jstr::Contains( 3, "-s ",      SettingLength, String ) );

  // -t u v w
  Assert( ! jstr::Contains( 3, "-t ",      SettingLength, String ) );

  // -texres value
  Assert( ! jstr::Contains( 8, "-texres ", SettingLength, String ) );
}

obj_mtl_data* ReadMTLFile(void* (*AssetAllocator)(u32 ByteSize), memory_arena* TempArena,  char* FileName)
{
  debug_read_file_result ReadResult = Platform.DEBUGPlatformReadEntireFile(FileName);

  if( !ReadResult.ContentSize ) { return 0; }

  char LineBuffer[STR_MAX_LINE_LENGTH] = {};

  char* ScanPtr = ( char* ) ReadResult.Contents;
  char* FileEnd =  ( char* ) ReadResult.Contents + ReadResult.ContentSize;

  fifo_queue<mtl_material*> ParsedMaterialQueue = fifo_queue<mtl_material*>(TempArena);
  mtl_material* ActieveMaterial = 0;
  while( ScanPtr < FileEnd )
  {
    midx Length = 0;
    ScanPtr = GetNextLine(ScanPtr, FileEnd, &Length, LineBuffer);
    if(Length == 0)
    {
      continue;
    }

    type_string_pair DataType = GetMTLDataType( Length, LineBuffer );

    switch(DataType.Enum)
    {
      case MTL_EMPTY:
      {

      }break;
      case MTL_NEW_MATERIAL:
      {
        ActieveMaterial = (mtl_material*) AssetAllocator(sizeof(mtl_material));
        ActieveMaterial->NameLength = jstr::StringLength( DataType.String );
        ActieveMaterial->Name = (char*) AssetAllocator((ActieveMaterial->NameLength+1) * sizeof(char));
        jstr::CopyStrings( ActieveMaterial->NameLength, DataType.String, ActieveMaterial->NameLength+1, ActieveMaterial->Name );
        ParsedMaterialQueue.Push(ActieveMaterial);

      }break;
      case MTL_KA:
      {
        // If line contains "spectral" it means that the material parameters are in a separate file.rfl "Ka spectral file.rfl factor"
        // The statement specifies the ambient reflectivity using a spectral curve.
        Assert( ! jstr::Contains( 8, "spectral", jstr::StringLength( DataType.String ), DataType.String ) );
        //The "Ka xyz" statement specifies the ambient reflectivity using CIEXYZ values.
        Assert( ! jstr::Contains( 3, "xyz", jstr::StringLength( DataType.String ), DataType.String ) );
        ActieveMaterial->Ka = (v4*) AssetAllocator(sizeof(v4));
        *ActieveMaterial->Ka = ParseNumbers(DataType.String);
        ActieveMaterial->Ka->W = 1;
      }break;
      case MTL_KD:
      {
        Assert( ! jstr::Contains( 8, "spectral", jstr::StringLength( DataType.String ), DataType.String ) );
        Assert( ! jstr::Contains( 3, "xyz", jstr::StringLength( DataType.String ), DataType.String ) );
        ActieveMaterial->Kd = (v4*) AssetAllocator(sizeof(v4));
        *ActieveMaterial->Kd = ParseNumbers(DataType.String);
        ActieveMaterial->Kd->W = 1;
      }break;
      case MTL_KS:
      {
        Assert( ! jstr::Contains( 8, "spectral", jstr::StringLength( DataType.String ), DataType.String ) );
        Assert( ! jstr::Contains( 3, "xyz", jstr::StringLength( DataType.String ), DataType.String ) );
        ActieveMaterial->Ks = (v4*) AssetAllocator(sizeof(v4));
        *ActieveMaterial->Ks = ParseNumbers(DataType.String);
        ActieveMaterial->Ks->W = 1;
      }break;
      case MTL_KE:
      {
        Assert( ! jstr::Contains( 8, "spectral", jstr::StringLength( DataType.String ), DataType.String ) );
        Assert( ! jstr::Contains( 3, "xyz", jstr::StringLength( DataType.String ), DataType.String ) );
        ActieveMaterial->Ke = (v4*) AssetAllocator(sizeof(v4));
        *ActieveMaterial->Ke = ParseNumbers(DataType.String);
        ActieveMaterial->Ke->W = 1;
      }break;
      case MTL_TF:
      {
        Assert( ! jstr::Contains( 8, "spectral", jstr::StringLength( DataType.String ), DataType.String ) );
        Assert( ! jstr::Contains( 3, "xyz", jstr::StringLength( DataType.String ), DataType.String ) );
        ActieveMaterial->Tf = (v4*) AssetAllocator(sizeof(v4));
        *ActieveMaterial->Tf = ParseNumbers(DataType.String);
        ActieveMaterial->Tf->W = 1;
      }break;
      case MTL_ILLUMINATION:
      {
        // Unimplemented
        ActieveMaterial->IlluminationMode = (u32*) AssetAllocator( sizeof(u32) );
        *ActieveMaterial->IlluminationMode = (u32) jstr::StringToReal64(DataType.String);
      }break;
      case MTL_D: // Specifies the dissolve for the current material, Example "d -halo 0.4", "d "
      {
        Assert( ! jstr::Contains( 8, "-halo", jstr::StringLength( DataType.String ), DataType.String ) );
        ActieveMaterial->d = (r32*) AssetAllocator( sizeof(r32) );
        *ActieveMaterial->d = (r32) jstr::StringToReal64(DataType.String);
      }break;
      case MTL_NS: // Specifies the specular exponent for the current material. This defines the focus of the specular highlight.
      {
        ActieveMaterial->Ns = (r32*) AssetAllocator( sizeof(r32) );
        *ActieveMaterial->Ns = (r32) jstr::StringToReal64(DataType.String);
      }break;
      case MTL_SHARPNESS:
      {
        Assert(0);
      }break;
      case MTL_NI: //Specifies the optical density for the surface. This is also known as index of refraction.
      {
        ActieveMaterial->Ni = (r32*) AssetAllocator( sizeof(r32) );
        *ActieveMaterial->Ni = (r32) jstr::StringToReal64(DataType.String);
      }break;
      case MTL_MAP_KA:
      {
        Assert(0);
      }break;
      case MTL_MAP_KD: // Diffuse Texture Map
      {

        // Unimplemented Settings
        AssertNoMapSettings(DataType.String);
        char TGAFilePath[STR_MAX_LINE_LENGTH] = {};
        CreateNewFilePath( FileName, DataType.String, TGAFilePath );

        ActieveMaterial->MapKd = LoadTGA(AssetAllocator, TGAFilePath);
      }break;
      case MTL_MAP_KS: // Specular Texture Map
      { 
        // Unimplemented Settings
        AssertNoMapSettings(DataType.String);

        char TGAFilePath[STR_MAX_LINE_LENGTH] = {};
        CreateNewFilePath( FileName, DataType.String, TGAFilePath );

        ActieveMaterial->MapKs = LoadTGA(AssetAllocator, TGAFilePath);
      }break;
      case MTL_MAP_NS:
      {
        // Unimplemented Settings
        AssertNoMapSettings(DataType.String);

        char TGAFilePath[STR_MAX_LINE_LENGTH] = {};
        CreateNewFilePath( FileName, DataType.String, TGAFilePath );

        ActieveMaterial->MapNs = LoadTGA(AssetAllocator, TGAFilePath);
      }break;
      case MTL_MAP_D:
      {
        Assert(0);
      }break;
      case MTL_DISP:
      {
        Assert(0);
      }break;
      case MTL_DECAL:
      {
        Assert(0);
      }break;
      case MTL_MAP_BUMP:
      {
        // Unimplemented Settings

        u32 SettingLength = jstr::StringLength( DataType.String );

        // -imfchan r | g | b | m | l | z
        Assert( ! jstr::Contains( 9, "-imfchan ", SettingLength, DataType.String ) );

        // -blendu on | off
        Assert( ! jstr::Contains( 8, "-blendu ",  SettingLength, DataType.String ) );

        // -blendv on | off
        Assert( ! jstr::Contains( 8, "-blendv ",  SettingLength, DataType.String ) );

        // -cc on | off
        Assert( ! jstr::Contains( 4, "-cc ",      SettingLength, DataType.String ) );

        // -clamp on | off
        Assert( ! jstr::Contains( 7, "-clamp ",   SettingLength, DataType.String ) );

        // -mm base gain
        Assert( ! jstr::Contains( 4, "-mm ",      SettingLength, DataType.String ) );

        // -o u v w
        Assert( ! jstr::Contains( 3, "-o ",       SettingLength, DataType.String ) );

        // -s u v w
        Assert( ! jstr::Contains( 3, "-s ",       SettingLength, DataType.String ) );

        // -t u v w
        Assert( ! jstr::Contains( 3, "-t ",       SettingLength, DataType.String ) );

        // -texres value
        Assert( ! jstr::Contains( 8, "-texres ",  SettingLength, DataType.String ) );

        char* SettingStartPtr = 0;
        // -bm mult, mult: [0,1]

        char LeftoverString[STR_MAX_LINE_LENGTH] = {};

        fifo_queue<char*> SettingQueue = ExtractMTLSettings( TempArena, 1, "-bm ",  " \t",  DataType.String,  LeftoverString );
        Assert(SettingQueue.GetSize() == 1);

        ActieveMaterial->BumpMapBM = (r32) jstr::StringToReal64( SettingQueue.Pop() );


        char TGAFilePath[STR_MAX_LINE_LENGTH] = {};
        CreateNewFilePath( FileName, LeftoverString, TGAFilePath );

        ActieveMaterial->BumpMap = LoadTGA( AssetAllocator, TGAFilePath);
      }break;
      case MTL_REFLECTION_MAP:
      {
        Assert(0);
      }break;
      default:
      {
        Assert(0);
      }
    }


  }

  mtl_material* Material = 0;
  obj_mtl_data* Result = (obj_mtl_data*) AssetAllocator(sizeof(obj_mtl_data));
  Result->MaterialCount = ParsedMaterialQueue.GetSize();
  Result->Materials = (mtl_material*) AssetAllocator( Result->MaterialCount * sizeof(mtl_material));
  Result->PathLength = jstr::StringLength(FileName);
  Result->Path = (char*) AssetAllocator((Result->PathLength+1) * sizeof(char));
  jstr::CopyStrings( Result->PathLength, FileName, Result->PathLength+1, Result->Path );
  mtl_material* MaterialPosition = Result->Materials;
  while( (Material = ParsedMaterialQueue.Pop()) != 0 )
  {
    *MaterialPosition = *Material;
    ++MaterialPosition;
  }

  Platform.DEBUGPlatformFreeFileMemory(ReadResult.Contents);

  return Result;
}

void SetBoundingBox( obj_loaded_file* OBJFile, memory_arena* TempArena )
{
  obj_mesh_data* MeshData = OBJFile->MeshData;
  for( u32 GroupIndex = 0; GroupIndex < OBJFile->ObjectCount; ++GroupIndex )
  {
    obj_group& OBJGroup = OBJFile->ObjectGroups[GroupIndex];

    obj_mesh_indeces* Indeces = OBJGroup.Indeces;

    if(Indeces->Count)
    {
      v3  Max = MeshData->v[Indeces->vi[0]];
      v3  Min = MeshData->v[Indeces->vi[0]];
      for( u32 Index=0; Index < Indeces->Count; ++Index )
      {
        u32 VertexIndex = Indeces->vi[Index];

        v3 Vertex = MeshData->v[VertexIndex];

        Max.X = Max.X > Vertex.X ? Max.X : Vertex.X;
        Max.Y = Max.Y > Vertex.Y ? Max.Y : Vertex.Y;
        Max.Z = Max.Z > Vertex.Z ? Max.Z : Vertex.Z;

        Min.X = Min.X < Vertex.X ? Min.X : Vertex.X;
        Min.Y = Min.Y < Vertex.Y ? Min.Y : Vertex.Y;
        Min.Z = Min.Z < Vertex.Z ? Min.Z : Vertex.Z;
      }

      OBJGroup.aabb = AABB3f(Min,Max);
    }
  }
}

void SetGroupName(group_to_parse* Group, memory_arena* TempArena, u32 GroupNameLength, c8* GroupName)
{
  Assert(Group->GroupName == 0);
  Group->GroupNameLength = GroupNameLength;
  Group->GroupName = (char*) PushArray(TempArena, GroupNameLength+1, char );
  jstr::CopyStrings(GroupNameLength,  GroupName, GroupNameLength, Group->GroupName );
}

group_to_parse* NewGroup(memory_arena* TempArena)
{
  group_to_parse* Result = (group_to_parse*) PushStruct(TempArena, group_to_parse);
  Result->vi = fifo_queue<u32>(TempArena);
  Result->ti = fifo_queue<u32>(TempArena);
  Result->ni = fifo_queue<u32>(TempArena);
  return Result;
}

obj_loaded_file* ReadOBJFile(void* (*AssetAllocator)(u32 ByteSize), memory_arena* TempArena, const char* FileName)
{
  debug_read_file_result ReadResult = Platform.DEBUGPlatformReadEntireFile(FileName);

  char LineBuffer[STR_MAX_LINE_LENGTH];

  if( !ReadResult.ContentSize ){ return {}; }

  fifo_queue<group_to_parse*> GroupToParse = fifo_queue<group_to_parse*>(TempArena);
  group_to_parse* DefaultGroup = (group_to_parse*) PushStruct(TempArena, group_to_parse);
  DefaultGroup->vi = fifo_queue<u32>(TempArena);
  DefaultGroup->ti = fifo_queue<u32>(TempArena);
  DefaultGroup->ni = fifo_queue<u32>(TempArena);

  group_to_parse* ActiveGroup = DefaultGroup;

  fifo_queue<v3> VerticeBuffer    = fifo_queue<v3>( TempArena );
  fifo_queue<v3> VerticeNormalBuffer  = fifo_queue<v3>( TempArena );
  fifo_queue<v2> TextureVerticeBuffer = fifo_queue<v2>( TempArena );

  u32 ObjectNameLength = 0;
  char* ObjectName = 0;

  obj_mtl_data* MaterialFile = 0;

  char* ScanPtr = ( char* ) ReadResult.Contents;
  char* FileEnd =  ( char* ) ReadResult.Contents + ReadResult.ContentSize;

  while( ScanPtr < FileEnd )
  {
    midx Length = 0;
    ScanPtr = GetNextLine(ScanPtr, FileEnd, &Length, LineBuffer);
    if(Length == 0)
    {
      continue;
    }

    type_string_pair DataType = GetOBJDataType( Length, LineBuffer );

    switch( DataType.Enum )
    {
      case obj_data_types::OBJ_EMPTY:
      {
        continue;
      }break;

      // Vertices: v x y z
      case obj_data_types::OBJ_GEOMETRIC_VERTICES:
      {
        v3 Vert = V3(ParseNumbers(DataType.String));
        VerticeBuffer.Push(Vert);

      }break;

      // Vertex Normals: vn i j k
      case obj_data_types::OBJ_VERTEX_NORMALS:
      {
        v3 VertNorm = V3(ParseNumbers(DataType.String));
        VerticeNormalBuffer.Push(VertNorm);
      }break;

      // Vertex Textures: vt u v
      case obj_data_types::OBJ_TEXTURE_VERTICES:
      {
        v2 TextureVertice = V2( ParseNumbers(DataType.String) );
        TextureVerticeBuffer.Push( TextureVertice );
      }break;

      // Faces: f v1[/vt1][/vn1] v2[/vt2][/vn2] v3[/vt3][/vn3] ...
      case obj_data_types::OBJ_FACE:
      {
        ParseFaceLine( TempArena, DataType.String, ActiveGroup );
      }break;

      // Group: name1 name2 ....
      case obj_data_types::OBJ_GROUP_NAME:
      {
        u32 GroupNameLength = jstr::StringLength( DataType.String );
        if( !jstr::Contains(7, "default", GroupNameLength, DataType.String ) )
        {
          // If we already have a group name and see a new one in the file, it should mean we want to parse a new group
          if(ActiveGroup->GroupName)
          {
            ActiveGroup = NewGroup(TempArena);
            GroupToParse.Push( ActiveGroup );
          }
          SetGroupName(ActiveGroup, TempArena, GroupNameLength, DataType.String);
        }
      }break;

      case obj_data_types::OBJ_MATERIAL_LIBRARY:
      {
        char MTLFileName[STR_MAX_LINE_LENGTH] = {};

        CreateNewFilePath( FileName, DataType.String, MTLFileName );

        if(!MaterialFile)
        {
          // Stores materials to Asset Arena
          MaterialFile = ReadMTLFile(AssetAllocator, TempArena, MTLFileName);
        }


      } break;
      case obj_data_types::OBJ_OBJECT_NAME:
      {
        // Unimplemented
        char* kek = DataType.String;
        // Only One object name per file i think
        Assert(!ObjectName || !ObjectNameLength);
        ObjectNameLength = jstr::StringLength( DataType.String );

        ObjectNameLength = ObjectNameLength;
        ObjectName = (char*) PushArray(TempArena, ObjectNameLength+1, char );
        jstr::CopyStrings(ObjectNameLength,  DataType.String, ObjectNameLength, ObjectName );
      } break;
      case obj_data_types::OBJ_MATERIAL_NAME:
      {
        u32 MaterialNameLength = jstr::StringLength( DataType.String );
#if 1
        if(ActiveGroup->MaterialName)
        {
          // Some files don't use OBJ_GROUP_NAME to announce a new render group, they just plootch down a new material.
          // Therefore if we already have a material in the active group and see a new one, we create a new active group.
          if(!ActiveGroup->GroupName){
            // But before that, if we don't have a group name yet, set a default one
            local_persist u32 Num = 0;
            if(Num == 13348){
              int a = 10;
            }

            c8 Buff[256] = {};
            u32 StrLength = FormatString(Buff, ArrayCount(Buff), "Group_%d", Num++ );
            SetGroupName(ActiveGroup, TempArena, StrLength, Buff);
          }
          ActiveGroup = NewGroup(TempArena);
          GroupToParse.Push( ActiveGroup );
        }
#endif
        ActiveGroup->MaterialNameLength = MaterialNameLength;
        ActiveGroup->MaterialName = (char*) PushArray(TempArena, MaterialNameLength+1, char );
        ActiveGroup->IndiceOffset = ActiveGroup->vi.GetSize();
        jstr::CopyStrings(MaterialNameLength,  DataType.String, MaterialNameLength, ActiveGroup->MaterialName );

      } break;
      case obj_data_types::OBJ_SMOOTHING_GROUP:
      {
        if(!jstr::Contains( 3, "off", jstr::StringLength( DataType.String ), DataType.String) && !jstr::Equals(DataType.String, "0"))
        {
          ActiveGroup->SmoothingGroup = (u32*) PushStruct(TempArena, u32);
          *ActiveGroup->SmoothingGroup = jstr::StringToReal64(DataType.String);
        }
      } break;
      case obj_data_types::OBJ_CURVE_OR_SURFACE_TYPE:
      case obj_data_types::OBJ_END_STATEMENT:
      case obj_data_types::OBJ_DEGREE:
      case obj_data_types::OBJ_CURVE:
      case obj_data_types::OBJ_PARAMETER_VALUES:
      {
        // These are datatypes ive encountered related to bezier curves.
        // I ignore these for now.

        // Down the line this becomes empty object_groups without any indeces. Once we add surfaces and splines etc we will have 
        // to rework the data structures somehow. For now we are okay with some memory being wasted holding empty objects.
      } break;


      default:
      {
        Assert(0);

      } break;
    }

  }

  if(ActiveGroup == DefaultGroup)
  {
    GroupToParse.Push( DefaultGroup );
  }

  Platform.DEBUGPlatformFreeFileMemory(ReadResult.Contents);

  obj_loaded_file* Result = (obj_loaded_file*) AssetAllocator( sizeof(obj_loaded_file) );

  Result->ObjectNameLength = ObjectNameLength;
  if(Result->ObjectNameLength > 0)
  {
    u32 NameSize = (ObjectNameLength+1)*sizeof(char);
    Result->ObjectName = (char*) AssetAllocator(NameSize);
    utils::Copy(NameSize, ObjectName, Result->ObjectName);
  }

  obj_mesh_data* MeshData = (obj_mesh_data*) AssetAllocator( sizeof(obj_mesh_data) );

  MeshData->nv = VerticeBuffer.GetSize();
  MeshData->v = (v3*) AssetAllocator(MeshData->nv * sizeof(v3));
  for( u32 Index = 0; Index < MeshData->nv; ++Index )
  {
    MeshData->v[Index] = VerticeBuffer.Pop();
  }
  Assert(VerticeBuffer.IsEmpty());


  MeshData->nvn = VerticeNormalBuffer.GetSize();
  MeshData->vn = (v3*) AssetAllocator(MeshData->nvn* sizeof(v3));
  for( u32 Index = 0; Index < MeshData->nvn; ++Index )
  {
    MeshData->vn[Index] = VerticeNormalBuffer.Pop();
  }
  Assert(VerticeNormalBuffer.IsEmpty());


  MeshData->nvt = TextureVerticeBuffer.GetSize();
  MeshData->vt = (v2*) AssetAllocator( MeshData->nvt * sizeof(v2));
  for( u32 Index = 0; Index < MeshData->nvt; ++Index )
  {
    MeshData->vt[Index] = TextureVerticeBuffer.Pop();
  }
  Assert(TextureVerticeBuffer.IsEmpty());


  Result->ObjectCount  = GroupToParse.GetSize();
  Result->ObjectGroups = (obj_group*) AssetAllocator(Result->ObjectCount* sizeof(obj_group));
  Result->MeshData     = MeshData;
  Result->MaterialData = MaterialFile;

  obj_group* NewGroup = Result->ObjectGroups;
  while( !GroupToParse.IsEmpty() )
  {
    group_to_parse* ParsedGroup = GroupToParse.Pop();

    NewGroup->GroupNameLength = ParsedGroup->GroupNameLength;
    NewGroup->GroupName = (char*) AssetAllocator( (ParsedGroup->GroupNameLength+1) * sizeof(char) );
    NewGroup->SmoothingGroup = ParsedGroup->SmoothingGroup;
    jstr::CopyStrings( ParsedGroup->GroupNameLength, ParsedGroup->GroupName, NewGroup->GroupNameLength, NewGroup->GroupName );
    NewGroup->Indeces = (obj_mesh_indeces*) AssetAllocator( sizeof(obj_mesh_indeces) );
    if(Result->MaterialData && ParsedGroup->MaterialName )
    {
      for( u32 MaterialIndex = 0; MaterialIndex < Result->MaterialData->MaterialCount; ++MaterialIndex)
      {
        mtl_material* mtl =  &Result->MaterialData->Materials[MaterialIndex];
        if( jstr::Equals(ParsedGroup->MaterialName, mtl->Name ) )
        {
          NewGroup->Material = mtl;
          NewGroup->Material->IndiceOffset = ParsedGroup->IndiceOffset;
        }

      }
    }

    obj_mesh_indeces* Indeces = NewGroup->Indeces;
    Indeces->Count = ParsedGroup->vi.GetSize();

    if(ParsedGroup->vi.GetSize())
    {
      Indeces->vi    = (u32*) AssetAllocator( Indeces->Count * sizeof(u32) );
      for( u32 i = 0; i < Indeces->Count; ++i )
      {
        Indeces->vi[i] = ParsedGroup->vi.Pop();
      }
    }

    if(ParsedGroup->ti.GetSize())
    {
      Indeces->ti    = (u32*) AssetAllocator( Indeces->Count * sizeof(u32) );
      for( u32 i = 0; i < Indeces->Count; ++i )
      {
        Indeces->ti[i] = ParsedGroup->ti.Pop();
      }
    }

    if(ParsedGroup->ni.GetSize())
    {
      Indeces->ni    = (u32*) AssetAllocator( Indeces->Count * sizeof(u32) );
      for( u32 i = 0; i < Indeces->Count; ++i )
      {
        Indeces->ni[i] = ParsedGroup->ni.Pop();
      }
    }

    NewGroup++;
  }

  Assert( GroupToParse.IsEmpty() );
  SetBoundingBox(Result, TempArena);

  return Result;
}



void FreeObjectGroup(void (*FreeMemory)(void* Data), obj_group* Grp)
{
  if(Grp->GroupName)
  {
    FreeMemory( (void*) Grp->GroupName);
    Grp->GroupNameLength = 0;
    Grp->GroupName = 0;
  }

  if(Grp->SmoothingGroup)
  {
    FreeMemory( (void*) Grp->SmoothingGroup);
    Grp->SmoothingGroup = 0;
  }

  if(Grp->Indeces)
  {
    obj_mesh_indeces* Indeces = Grp->Indeces;
    if(Indeces->vi)
    {
      FreeMemory( (void*) Indeces->vi);
    }
    if(Indeces->ti)
    {
      FreeMemory( (void*) Indeces->ti);
    }
    if(Indeces->ni)
    {
      FreeMemory( (void*) Indeces->ni);
    }
    FreeMemory( (void*) Indeces);
    Grp->Indeces = 0;
  }

  // Materials are held in the obj_loaded_file and will be freed from there.
}

void FreeMeshData(void (*FreeMemory)(void* Data), obj_mesh_data* Mesh)
{
  if(Mesh->v)
  {
    FreeMemory( (void*) Mesh->v);
  }
  if(Mesh->vn)
  {
    FreeMemory( (void*) Mesh->vn);
  }
  if(Mesh->vt)
  {
    FreeMemory( (void*) Mesh->vt);
  }

  FreeMemory( (void*) Mesh);
}

void FreeBitmap(void (*FreeMemory)(void* Data), obj_bitmap* Bitmap)
{
  if(Bitmap->Pixels)
  {
    FreeMemory( (void*) Bitmap->Pixels);
  }
  FreeMemory( (void*) Bitmap);
}

void FreeMaterialData(void (*FreeMemory)(void* Data), mtl_material* Mtl)
{
  if(Mtl->Name){
    FreeMemory( (void*) Mtl->Name);
    Mtl->Name = 0;
    Mtl->NameLength = 0;
  }

  if(Mtl->Kd){
    FreeMemory( (void*) Mtl->Kd);
    Mtl->Kd = 0;
  }
  if(Mtl->Ka){
    FreeMemory( (void*) Mtl->Ka);
    Mtl->Ka = 0;
  }
  if(Mtl->Tf){
    FreeMemory( (void*) Mtl->Tf);
    Mtl->Tf = 0;
  }
  if(Mtl->Ks){
    FreeMemory( (void*) Mtl->Ks);
    Mtl->Ks = 0;
  }
  if(Mtl->Ke){
    FreeMemory( (void*) Mtl->Ke);
    Mtl->Ke = 0;
  }
  if(Mtl->d){
    FreeMemory( (void*) Mtl->d);
    Mtl->d = 0;
  }
  if(Mtl->Ni){
    FreeMemory( (void*) Mtl->Ni);
    Mtl->Ni = 0;
  }
  if(Mtl->Ns){
    FreeMemory( (void*) Mtl->Ns);
    Mtl->Ns = 0;
  }

  if(Mtl->IlluminationMode){
    FreeMemory( (void*) Mtl->IlluminationMode);
    Mtl->IlluminationMode = 0;
  }

  if(Mtl->BumpMap)
  {
    FreeBitmap(FreeMemory, Mtl->BumpMap);
    Mtl->BumpMap = 0;
  }
  if(Mtl->MapKd)
  {
    FreeBitmap(FreeMemory, Mtl->MapKd);
    Mtl->MapKd = 0;
  }
  if(Mtl->MapKs)
  {
    FreeBitmap(FreeMemory, Mtl->MapKs);
    Mtl->MapKs = 0;
  }
}

void FreeObj(void (*FreeMemory)(void* Data), obj_loaded_file* Obj) {
  if(Obj->ObjectName)
  {
    FreeMemory((void*)Obj->ObjectName);
  }
  for (int i = 0; i < Obj->ObjectCount; ++i)
  {
    obj_group* ObjectGroup = Obj->ObjectGroups + i;
    FreeObjectGroup(FreeMemory, ObjectGroup);
  }

  if(Obj->MeshData)
  {
    FreeMeshData(FreeMemory, Obj->MeshData);
    Obj->MeshData = 0;
  }


  if(Obj->MaterialData)
  {
    obj_mtl_data* MaterialData = Obj->MaterialData;
    for (int i = 0; i < MaterialData->MaterialCount; ++i)
    {  
      mtl_material* Mtl = MaterialData->Materials + i;
      FreeMaterialData(FreeMemory, Mtl);
    }
    FreeMemory(MaterialData);
    Obj->MaterialData = 0;
  }

  FreeMemory(Obj);
}