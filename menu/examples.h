// Examples:
// Creating menu
GlobalGameState->MenuInterface = CreateMenuInterface(GlobalPersistentArena, Megabytes(1));
{
    ButtonSize.W += 0.02f;
    ButtonSize.H += 0.02f;
    r32 ContainerWidth = 0.7;
    container_node* SettingsPlugin = 0;
    {
      // Create Option Window
      container_node* ButtonContainer =  NewContainer(GlobalGameState->MenuInterface, container_type::Grid);
      grid_node* Grid = GetGridNode(ButtonContainer);
      Grid->Col = 1;
      Grid->Row = 0;
      Grid->TotalMarginX = 0.0;
      Grid->TotalMarginY = 0.0;

      color_attribute* BackgroundColor = (color_attribute* ) PushAttribute(GlobalGameState->MenuInterface, ButtonContainer, ATTRIBUTE_COLOR);
      BackgroundColor->Color = V4(0,0,0,0.7);

      auto CreateButton = [&ButtonSize, &FontSize, &TextColor]( b32* ButtonFlag, c8* ButtonText)
      {
        container_node* ButtonNode = NewContainer(GlobalGameState->MenuInterface);
        text_attribute* Text = (text_attribute*) PushAttribute(GlobalGameState->MenuInterface, ButtonNode, ATTRIBUTE_TEXT);
        Assert(str::StringLength( ButtonText ) < ArrayCount(Text->Text) );
        str::CopyStringsUnchecked( ButtonText, Text->Text );
        Text->FontSize = FontSize;
        Text->Color = TextColor;

        size_attribute* SizeAttr = (size_attribute*) PushAttribute(GlobalGameState->MenuInterface, ButtonNode, ATTRIBUTE_SIZE);
        SizeAttr->Width = ContainerSizeT(menu_size_type::ABSOLUTE_, ButtonSize.W);
        SizeAttr->Height = ContainerSizeT(menu_size_type::ABSOLUTE_, ButtonSize.H);
        SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::RELATIVE_, 0);
        SizeAttr->TopOffset = ContainerSizeT(menu_size_type::RELATIVE_, 0);
        SizeAttr->XAlignment = menu_region_alignment::CENTER;
        SizeAttr->YAlignment = menu_region_alignment::CENTER;

        v4 InactiveColor = V4(0.3,0.1,0.1,1);
        v4 ActiveColor = V4(0.1,0.3,0.1,1);

        color_attribute* ColorAttr = (color_attribute*) PushAttribute(GlobalGameState->MenuInterface, ButtonNode, ATTRIBUTE_COLOR);
        ColorAttr->Color = *ButtonFlag ?  ActiveColor : InactiveColor;

        RegisterMenuEvent(GlobalGameState->MenuInterface, menu_event_type::MouseDown, ButtonNode, ButtonFlag, DebugToggleButton, 0 );

        return ButtonNode;
      };

      ConnectNodeToBack(ButtonContainer, CreateButton(&DebugState->ConfigMultiThreaded, "Multithread"));
      ConnectNodeToBack(ButtonContainer, CreateButton(&DebugState->ConfigCollisionPoints, "CollisionPoints"));
      ConnectNodeToBack(ButtonContainer, CreateButton(&DebugState->ConfigCollider, "Colliders"));
      ConnectNodeToBack(ButtonContainer, CreateButton(&DebugState->ConfigAABBTree, "AABBTree"));
      container_node* RecompileButton = ConnectNodeToBack(ButtonContainer, NewContainer(GlobalGameState->MenuInterface));
      {
        color_attribute* Color = (color_attribute*) PushAttribute(GlobalGameState->MenuInterface, RecompileButton, ATTRIBUTE_COLOR);
        Color->Color = V4(0.2,0.1,0.3,1);
        
        text_attribute* Text = (text_attribute*) PushAttribute(GlobalGameState->MenuInterface, RecompileButton, ATTRIBUTE_TEXT);
        str::CopyStringsUnchecked( "Recompile", Text->Text );
        Text->FontSize = FontSize;
        Text->Color = TextColor;

        size_attribute* SizeAttr = (size_attribute*) PushAttribute(GlobalGameState->MenuInterface, RecompileButton, ATTRIBUTE_SIZE);
        SizeAttr->Width = ContainerSizeT(menu_size_type::ABSOLUTE_, ButtonSize.W);
        SizeAttr->Height = ContainerSizeT(menu_size_type::RELATIVE_, 1);
        SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::RELATIVE_, 0);
        SizeAttr->TopOffset = ContainerSizeT(menu_size_type::RELATIVE_, 0);
        SizeAttr->XAlignment = menu_region_alignment::CENTER;
        SizeAttr->YAlignment = menu_region_alignment::CENTER;

        RegisterMenuEvent(GlobalGameState->MenuInterface, menu_event_type::MouseDown, RecompileButton, 0, DebugRecompileButton, 0 );
      }
      SettingsPlugin = CreatePlugin(GlobalGameState->MenuInterface, "Settings", V4(0.5,0.5,0.5,1), ButtonContainer);
    }

    // Create graph window
    container_node* FunctionPlugin = 0;
    {

      container_node* FunctionContainer = NewContainer(GlobalGameState->MenuInterface);
      FunctionContainer->Functions.Draw = DeclareFunction(menu_draw, DrawStatistics);

      FunctionPlugin = CreatePlugin(GlobalGameState->MenuInterface, "Functions", HexCodeToColorV4( 0xABF74F ), FunctionContainer);
      color_attribute* BackgroundColor = (color_attribute* ) PushAttribute(GlobalGameState->MenuInterface, FunctionPlugin, ATTRIBUTE_COLOR);
      BackgroundColor->Color = V4(0,0,0,0.7);
    }

    // Create graph window
    container_node* GraphPlugin = 0;
    {
      container_node* ChartContainer = NewContainer(GlobalGameState->MenuInterface);
      ChartContainer->Functions.Draw = DeclareFunction(menu_draw, DrawFunctionTimeline);

      container_node* FrameContainer = NewContainer(GlobalGameState->MenuInterface);
      FrameContainer->Functions.Draw = DeclareFunction(menu_draw, DrawFrameFunctions);

      container_node* SplitNode  = NewContainer(GlobalGameState->MenuInterface, container_type::Split);
      color_attribute* BackgroundColor = (color_attribute* ) PushAttribute(GlobalGameState->MenuInterface, SplitNode, ATTRIBUTE_COLOR);
      BackgroundColor->Color = V4(0,0,0,0.7);

      container_node* BorderNode = CreateBorderNode(GlobalGameState->MenuInterface, false, 0.7);
      ConnectNodeToBack(SplitNode, BorderNode);
      ConnectNodeToBack(SplitNode, FrameContainer);
      ConnectNodeToBack(SplitNode, ChartContainer);

      GraphPlugin = CreatePlugin(GlobalGameState->MenuInterface, "Profiler", HexCodeToColorV4( 0xAB274F ), SplitNode);
    }

    menu_tree* WindowsDropDownMenu = RegisterMenu(GlobalGameState->MenuInterface, "Windows");
    RegisterWindow(GlobalGameState->MenuInterface, WindowsDropDownMenu, SettingsPlugin);
    RegisterWindow(GlobalGameState->MenuInterface, WindowsDropDownMenu, GraphPlugin);
    RegisterWindow(GlobalGameState->MenuInterface, WindowsDropDownMenu, FunctionPlugin);
    //ToggleWindow(GlobalGameState->MenuInterface, "Functions");
    //ToggleWindow(GlobalGameState->MenuInterface, "Settings");
    //ToggleWindow(GlobalGameState->MenuInterface, "Profiler");
}


// Drawing menu

MENU_DRAW(DrawFunctionTimeline)
{
  TIMED_FUNCTION();
  debug_state* DebugState = DEBUGGetState();

  rect2f Chart = Node->Region;

  r32 dt = GlobalGameState->Input->dt;

  r32 FrameTargetHeight = Chart.H * 0.7f;
  r32 HeightScaling = FrameTargetHeight/dt;

  
  u32 MaxFramesToDisplay = ArrayCount(DebugState->Frames)-1;

  game_window_size WindowSize = GameGetWindowSize();
  r32 PixelSize = 1.f / WindowSize.HeightPx;

  r32 BarGap = PixelSize;

  v4 RedColor = V4(1,0,0,1);
  r32 FullRed = 1.2;
  r32 FullBlue = 1;
  v4 BlueColor =  V4(0,0,1,1);

  r32 MouseX = Interface->MousePos.X;
  r32 MouseY = Interface->MousePos.Y;

  u32 FrameCount =  ArrayCount(DebugState->Frames);
  u32 Count = 0;
  debug_frame* Frame = DebugState->Frames + DebugState->CurrentFrameIndex+1;
  debug_frame* SelectedFrame = 0;
  debug_thread* SelectedThread = 0;
  r32 BarWidth = Chart.W/(MaxFramesToDisplay);
  for(u32 BarIndex = 0; BarIndex < MaxFramesToDisplay; ++BarIndex)
  {
    u32 FrameIndex = (u32) (Frame-DebugState->Frames);
    if(FrameIndex >= FrameCount)
    {
      Frame = DebugState->Frames; 
    }
    
    debug_block* Block = Frame->Threads[0].FirstBlock;

    r32 FrameY = Chart.Y;

    r32 FrameHitRatio = Frame->WallSecondsElapsed / dt;
    r32 FrameCycleScaling = FrameTargetHeight * FrameHitRatio/(Frame->EndClock - Frame->BeginClock);

    r32 FrameX = Chart.X + (r32)BarIndex*BarWidth;

    if(DebugState->SelectedFrame && DebugState->SelectedFrame == Frame)
    {
      u64 LaneCycleCount = GetBlockChainCycleCount(Block);
      rect2f Rect = {};
      Rect.X = FrameX - 0.5f*PixelSize;
      Rect.Y = FrameY - 0.5f*PixelSize;
      Rect.W = BarWidth + PixelSize;
      Rect.H = FrameCycleScaling*LaneCycleCount + 2*PixelSize;
      PushOverlayQuad(Rect, V4(1,1,0,1));
    }

    while(Block)
    {
      rect2f Rect = {};
      Rect.X = FrameX + BarGap * 0.5f;
      Rect.Y = FrameY+FrameCycleScaling*Block->BeginClock;
      Rect.W = BarWidth - BarGap;
      Rect.H = FrameCycleScaling*(Block->EndClock - Block->BeginClock);

      v4 Color = GetColorForRecord(Block->Record);
      PushOverlayQuad(Rect, Color);
      
      if(Intersects(Rect, V2(MouseX,MouseY)))
      {
        c8 StringBuffer[2048] = {};
        Platform.DEBUGFormatString( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer),
        "Frame %d: %2.2f Sec (%s)", FrameIndex, Frame->WallSecondsElapsed, Block->Record->BlockName);

        PushOverlayTextAt(MouseX, MouseY+0.02f, StringBuffer, 24, V4(1,1,1,1));
      }
      Block = Block->Next;
    }  

    rect2f FrameRect = {};
    FrameRect.X = FrameX;
    FrameRect.Y = FrameY;
    FrameRect.W = BarWidth;
    FrameRect.H = Frame->WallSecondsElapsed * HeightScaling;

    if(Intersects(FrameRect, V2(MouseX,MouseY)))
    {
      SelectedFrame = Frame;
    }
    ++Frame;
  }

  if(Interface->MouseLeftButton.Edge && Interface->MouseLeftButton.Active)
  {
    if(SelectedFrame)
    {
      DebugState->SelectedFrame = SelectedFrame;
      DebugState->Paused = true;
    }else if(Intersects(Chart, V2(MouseX,MouseY))){
      DebugState->SelectedFrame = 0;
      DebugState->Paused = false;
    }
  }
  

  rect2f Rect = {};
  Rect.X = Chart.X;
  Rect.Y = Chart.Y + FrameTargetHeight - 0.001f*0.5f;
  Rect.W = Chart.W;
  Rect.H = 0.001;
  PushOverlayQuad(Rect, V4(0,0,0,1));
}