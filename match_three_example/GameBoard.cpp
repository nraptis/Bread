#include "Game.h"
#include "GameBoard.h"

GameBoard *gBoard = 0;

GameBoard::GameBoard()
{
    gBoard = this;
    
    mTileWidth = gTileWidth;
    mTileHeight = gTileHeight;
    
    SetFrame(0.0f, 0.0f, gAppWidth, gAppHeight);
    
    mLayoutWidth = -1.0f;
    mLayoutHeight = -1.0f;
    
    mTile = 0;
    
    mGridWidth = 0;
    mGridHeight = 0;
    
    mGridWidth = 0;
    mGridHeight = 0;
    
    mOffsetX = 0.0f;
    mOffsetY = 0.0f;
    
    mMatchTypeCount = 3;
    mMatchRequiredCount = 3;
    
    mMatchAllowDiagonal = true;
    
    mMatch = 0;
    
    mMatchCount = 0;
    mMatchSize = 0;
    
    mLayerMatchType = 0;
    mLayerMatchCheck = 0;
    mLayerMatchFlagged = 0;
    mLayerMatchCount = 0;
    mLayerMatchStackX = 0;
    mLayerMatchStackY = 0;
    mLayerHoles = 0;
    
    
    for(int aActionIndex=0;aActionIndex<BA_LIST_COUNT;aActionIndex++)
    {
        mActionTick[aActionIndex] = 0;
        mActionTickDefault[aActionIndex] = 1;
        mActionRequested[aActionIndex] = false;
        
        //mActionAutoRequestCount[aActionIndex] = 0;
        
        //for(int i=0;i<BA_LIST_AUTO_COUNT;i++)mActionAutoRequest[aActionIndex][i] = 0;
        mActionDrawTick[aActionIndex] = 0;
    }
    
    mActionTickDefault[BA_TOPPLE] = 2;
    mActionTickDefault[BA_SWAP] = 3;
    mActionTickDefault[BA_POWER_UP] = 10;
    mActionTickDefault[BA_BONUS] = 3;
    mActionTickDefault[BA_CASCADE_CHECK] = 3;
    mActionTickDefault[BA_DESTROY] = 3;
    mActionTickDefault[BA_ZAP_ITEM] = 200;
    
    Reset();
}

GameBoard::~GameBoard()
{
    //Log("GameBoard::~GameBoard()\n");
    
    Free();
    gBoard = 0;
}



void GameBoard::Update()
{
    if(mSwap)
    {
        ActionActivate(BA_SWAP);
        
        mSwapTimer--;
        
        float aCX1 = GetTileCenterX(mSwapTileGridX[0]);
        float aCY1 = GetTileCenterY(mSwapTileGridY[0]);
        
        float aCX2 = GetTileCenterX(mSwapTileGridX[1]);
        float aCY2 = GetTileCenterY(mSwapTileGridY[1]);
        
        if(mSwapTimer <= 0)
        {
            mSwap = false;
            mSwapTimer = 0;
            mSwapTile[0] = 0;
            mSwapTile[1] = 0;
            
            GameTileMatchable *aSwapTile1 = GetTileMatchable(mSwapTileGridX[0], mSwapTileGridY[0]);
            GameTileMatchable *aSwapTile2 = GetTileMatchable(mSwapTileGridX[1], mSwapTileGridY[1]);
            
            if(aSwapTile1)aSwapTile1->AnimationDeselect();
            if(aSwapTile2)aSwapTile1->AnimationDeselect();
            
            RemoveTile(mSwapTileGridX[0], mSwapTileGridY[0]);
            RemoveTile(mSwapTileGridX[1], mSwapTileGridY[1]);
            
            SetTile(mSwapTileGridX[0], mSwapTileGridY[0], aSwapTile2);
            SetTile(mSwapTileGridX[1], mSwapTileGridY[1], aSwapTile1);
            
            
            
            bool aMatch = false;
            
            
            MatchComputeSetup();
            if(MatchCompute(mSwapTileGridX[0], mSwapTileGridY[0]))aMatch = true;
            if(MatchCompute(mSwapTileGridX[1], mSwapTileGridY[1]))aMatch = true;
            
            //if(MatchComputeV(mSwapTileGridX[0], mSwapTileGridY[0]))aMatch = true;
            //if(MatchComputeV(mSwapTileGridX[1], mSwapTileGridY[1]))aMatch = true;
            
            Log("_____Mat[%s][%d]______\n_________________\n", (aMatch ? "yes" : "no"), mMatchCount);
            
            if(mSwapReverse == false)
            {
                if(aMatch == false)
                {
                    if(SwapTilesAllowed(mSwapTileGridX[1], mSwapTileGridY[1], mSwapTileGridX[0], mSwapTileGridY[0]))
                    {
                        SwapTiles(mSwapTileGridX[0], mSwapTileGridY[0], mSwapTileGridX[1], mSwapTileGridY[1], true);
                    }
                }
                else
                {
                    HandleMatches();
                }
            }
            
            if(mSwap == false)
            {
                SwapTilesCancel();
            }
            
            
            
            
            
        }
        else
        {
            float aPercent = ((float)mSwapTimer) / ((float)SWAP_TIME);
            aPercent = FAnimation::EaseInSine(aPercent);
            
            mSwapTile[0]->mCenterX = aCX2 + (aCX1 - aCX2) * aPercent;
            mSwapTile[0]->mCenterY = aCY2 + (aCY1 - aCY2) * aPercent;
            mSwapTile[1]->mCenterX = aCX1 + (aCX2 - aCX1) * aPercent;
            mSwapTile[1]->mCenterY = aCY1 + (aCY2 - aCY1) * aPercent;
        }
    }
    
    GameTile *aTile = 0;
    //GameTileMatchable *aMatchTile = 0;
    
    bool aAnyDestroying = false;
    bool aAnyFalling = false;
    bool aAnyLanded = false;
    
    bool aAllDestroyingComplete = true;
    
    bool aWasFalling = false;
    
    for(int n=mGridHeight-1;n>=0;n--)
    {
        for(int i=0;i<mGridWidth;i++)
        {
            aTile = mTile[i][n];
            
            if(aTile)
            {
                aWasFalling = false;
                
                if(aTile->mFalling)
                {
                    aAnyFalling = true;
                    aWasFalling = true;
                }
                
                aTile->Update();
                
                if((aWasFalling == true) && (aTile->mFalling == false))
                {
                    if(aTile->mTileType == GAME_TILE_TYPE_MATCHABLE)
                    {
                        ((GameTileMatchable *)aTile)->AnimationLand();
                    }
                    aAnyLanded = true;
                }
                if(aTile->mFalling == true)
                {
                    aAnyFalling = true;
                }
                
                if(aTile->mDestroyed == true)
                {
                    aAnyDestroying = true;
                    
                    if(mTile[i][n]->mDestroyTimer > 0)
                    {
                        mTile[i][n]->mDestroyTimer--;
                        aAllDestroyingComplete = false;
                    }
                }
            }
        }
    }
    
    if(aAnyFalling)
    {
        ActionActivate(BA_TOPPLE);
    }
    
    if(aAnyDestroying == true)
    {
        ActionActivate(BA_DESTROY);
        
        if(aAllDestroyingComplete == false)
        {
            //ActionActivate(BA_MATCH);
            
            
            //ActionRequest(BA_DESTROY);
            //ActionRequest(BA_TOPPLE);
            
        }
        else
        {
            if(IsActionRequested(BA_DESTROY))
            {
                if(AllowDestroy())
                {
                    if(HandleDestroy())
                    {
                        ActionRequest(BA_TOPPLE);
                        
                        //ActionActivate(BA_TOPPLE);
                        //Topple();
                        
                        
                    }
                }
            }
            
            //if(HandleMatches())
            //{
            //    ActionRequest(BA_TOPPLE);
            //}
        }
        
        
    }
    
    else
    {
        if(aAnyFalling == false)
        {
            
            if(IsActionRequested(BA_TOPPLE))
            {
                if(AllowTopple())Topple();
            }
            
            if(IsActionRequested(BA_CASCADE_CHECK) == true)
            {
                if(AllowCascadeCheck())HandleCascadeCheck();
            }
            
            //
            
            //
        }
    }
    
    EnumList(GameTile, aTileDelete, mTilesDeleted)
    {
        aTileDelete->mTimerDelete--;
        if(aTileDelete->mTimerDelete <= 0)
        {
            mTilesDeletedThisUpdate.Add(aTileDelete);
        }
    }
    EnumList(GameTile, aTileDelete, mTilesDeletedThisUpdate)
    {
        mTilesDeleted.Remove(aTileDelete);
        delete aTileDelete;
    }
    mTilesDeletedThisUpdate.mCount = 0;
    
    mAnimations.Update();
    mAnimationsAdditive.Update();
    
    
    for(int i=0;i<BA_LIST_COUNT;i++)
    {
        if(mActionTick[i] > 0)
        {
            
            mActionTick[i]--;
            
            if(mActionTick[i] == 0)
            {
                Log("CooldownComp[%s]\n", ActionName(i).c());
            }
            
            
            mActionDrawTick[i] = 60;
            
            
        }
        else
        {
            if(mActionDrawTick[i] > 0)
            {
                mActionDrawTick[i]--;
            }
        }
    }
    
    
    
    if(mEffectTick <= 0)
    {
        GameTileMatchable *aEffectTile = 0;
        
        //;
        
        int aRand1 = gRand.Get(6);
        int aRand2 = gRand.Get(6);
        
        if(true)
        {
        for(int i=0;((i<3) && (aEffectTile==0));i++)
        {
            if(mEffectTwinkleCooldown[i] == 0)
            {
                aEffectTile = GetRandomMatchableTile();
                if(aEffectTile)
                {
                    aEffectTile->AnimationSparkle();
                    
                    mEffectTwinkleCooldown[i] = 200 + gRand.Get(300) + gRand.Get(100);
                    
                }
                
            }
        }
        }
        
        mEffectTick = 40 + gRand.Get(40);
        
    }
    else
    {
        mEffectTick--;
    }
    
    //mEffectTick = 0;
    
    for(int i=0;i<3;i++)
    {
        if(mEffectTwinkleCooldown[i] > 0)mEffectTwinkleCooldown[i]--;
        if(mEffectWiggleCooldown[i] > 0)mEffectWiggleCooldown[i]--;
        
    }
    
    //mEffectTwinkleCooldown[0] = 0;
    //mEffectTwinkleCooldown[1] = 0;
    //mEffectTwinkleCooldown[2] = 0;
    
    
    //mEffectWiggleCooldown[0] = 0;
    //mEffectWiggleCooldown[1] = 0;
   //mEffectWiggleCooldown[2] = 0;
    
    
    //ProcessRequestedActions();
}



void GameBoard::Draw()
{
    DrawTransform();

    DrawTiles();
    
    Graphics::SetColor();
    
    mAnimations.Draw();
    Graphics::BlendSetAdditive();Graphics::SetColor();
    mAnimationsAdditive.Draw();
    Graphics::BlendSetAlpha();Graphics::SetColor();
    
    Graphics::SetColor(1.0f, 0.0f, 0.45f);
    
    Graphics::OutlineRect(0.0f, 0.0f, mWidth, mHeight, 4.0f);
    
    
    
    
    /*
    float aRenderX = -300.0f;
    float aRenderY = -20.0f;
    
    for(int aActionIndex=0;aActionIndex<BA_LIST_COUNT;aActionIndex++)
    {
        FString aActionName = ActionName(aActionIndex);
        
        if(aActionName.mLength > 0)
        {
            FString aCooldown = FString(mActionTick[aActionIndex]);
            if(aCooldown.mLength < 2)aCooldown = FString(FString("0") + FString(aCooldown.c())).c();
            if(aCooldown.mLength < 2)aCooldown = FString(FString("0") + FString(aCooldown.c())).c();
            
            if(IsActionRequested(aActionIndex)) aActionName = FString(FString(aActionName.c()) + FString("+_*><[]()").c());
            
            if(mActionTick[aActionIndex] > 0)Graphics::SetColor(1.0f, 0.25f, 0.25f);
            else if(mActionDrawTick[aActionIndex] > 0)
            {
                float aPercent = (((float)mActionDrawTick[aActionIndex]) / 30.0f);
                float aPercentInv = (1.0f - aPercent);
                
                Graphics::SetColor(1.0f, 0.25f + aPercentInv * 0.75f, 0.25f + aPercentInv * 0.75f);
            }
            else Graphics::SetColor();
            
            
            gApp->mFontScoreLarge.Draw(aActionName.c(), aRenderX + 8.0f, aRenderY);
            gApp->mFontDialogHeader.Right(aCooldown.c(), aRenderX - 5.0f, aRenderY + 4.0f);
            
            
            aRenderY += 80.0f;
        }
        
        
        
    }
    */
    
    Graphics::SetColor();
    
    
}

void GameBoard::TouchDown(float pX, float pY, void *pData)
{
    int aGridX = GetTouchGridX(pX);
    int aGridY = GetTouchGridY(pY);
    
    if(AllowSelect())
    {
        GameTileMatchable *aSwapTile = GetTileMatchable(aGridX, aGridY);
        
        if(aSwapTile)
        {
            if(aGridX == 0){aSwapTile->AnimationLand();SwapTilesCancel();return;}
            if(aGridX == (mGridWidth - 1)){aSwapTile->AnimationMatch();SwapTilesCancel();return;}
            //if(aGridX == (mGridWidth - 1))aSwapTile->AnimationMatch();
            //aSwapTile->AnimationMatch();
        }
        
        if(aSwapTile != 0)
        {
            if(mSwapTile[0] == 0)
            {
                mSwapTile[0] = aSwapTile;
                mSwapTileGridX[0] = aGridX;
                mSwapTileGridY[0] = aGridY;
                
                mSwapDragStartX = pX;
                mSwapDragStartY = pY;
                mSwapDragData = pData;
                
                aSwapTile->AnimationSelect();
            }
            else
            {
                if(aSwapTile == mSwapTile[0])
                {
                    SwapTilesCancel();
                }
                else
                {
                    if(SwapTilesAllowed(mSwapTileGridX[0], mSwapTileGridY[0], aGridX, aGridY))
                    {
                        SwapTiles(mSwapTileGridX[0], mSwapTileGridY[0], aGridX, aGridY, false);
                    }
                    else
                    {
                        SwapTilesCancel();
                    }
                }
            }
        }
    }
}

void GameBoard::TouchMove(float pX, float pY, void *pData)
{
    if((mSwapTile[0] != 0) && (mSwapTile[1] == 0))
    {
        if((AllowSelect() == true) && (pData == mSwapDragData))
        {
            float aThreshold = 40.0f;
            
            bool aTrySwap = false;
            int aTrySwapDirX = 0;
            int aTrySwapDirY = 0;
            float aDiffX = pX - mSwapDragStartX;
            float aDiffY = pY - mSwapDragStartY;
            float aAbsDiffX = aDiffX;
            if(aAbsDiffX < 0)aAbsDiffX = -aDiffX;
            float aAbsDiffY = aDiffY;
            if(aAbsDiffY < 0)aAbsDiffY = -aDiffY;
            
            
            
            if(aAbsDiffX > aAbsDiffY)
            {
                if(aAbsDiffX >= aThreshold)
                {
                    aTrySwap = true;
                    if(aDiffX < 0)aTrySwapDirX = -1;
                    else aTrySwapDirX = 1;
                }
            }
            else
            {
                if(aAbsDiffY >= aThreshold)
                {
                    aTrySwap = true;
                    if(aDiffY < 0)aTrySwapDirY = -1;
                    else aTrySwapDirY = 1;
                }
            }
            
            if(aTrySwap == true)
            {
                if(SwapTilesAllowed(mSwapTileGridX[0], mSwapTileGridY[0], mSwapTileGridX[0] + aTrySwapDirX, mSwapTileGridY[0] + aTrySwapDirY))
                {
                    SwapTiles(mSwapTileGridX[0], mSwapTileGridY[0], mSwapTileGridX[0] + aTrySwapDirX, mSwapTileGridY[0] + aTrySwapDirY, false);
                }
            }
        }
    }
}

void GameBoard::TouchUp(float pX, float pY, void *pData)
{
    int aGridX = GetTouchGridX(pX);
    int aGridY = GetTouchGridY(pY);
    
    if((mSwapTile[0] != 0) && (mSwapTile[1] == 0))
    {
        if((AllowSelect() == true) && (pData == mSwapDragData))
        {
            GameTileMatchable *aSwapTile = GetTileMatchable(aGridX, aGridY);
            if((aSwapTile != 0) && (aSwapTile != mSwapTile[0]))
            {
                if(SwapTilesAllowed(mSwapTileGridX[0], mSwapTileGridY[0], aGridX, aGridY))
                {
                    SwapTiles(mSwapTileGridX[0], mSwapTileGridY[0], aGridX, aGridY, false);
                }
                else
                {
                    SwapTilesCancel();
                }
                
            }
            
        }
    }
}



void GameBoard::TouchFlush()
{
    
}

void GameBoard::LayoutContent(float pX, float pY, float pWidth, float pHeight)
{
    //Log("GameBoard::LayoutContent(%.2f, %.2f) (W:%.2f, H:%.2f)\n", pX, pY, pWidth, pHeight);
    
    Free();
    
    SetFrame(pX, pY, pWidth, pHeight);
    SizeGrid(8, 8);
    
    gRand.Seed(6345348);
    
    int aHoleCount = 1 + gRand.Get(3);
    
    for(int aHoleIndex = 0;aHoleIndex<aHoleCount;aHoleIndex++)
    {
        int aHoleX = gRand.Get(mGridWidth);
        int aHoleY = gRand.Get(mGridHeight);
        
        int aHoleWidth = 1 + gRand.Get(3);
        int aHoleHeight = 1 + gRand.Get(3);
        
        if(gRand.Get(4) == 1)
        {
            if(aHoleWidth < 3)aHoleWidth++;
            if(aHoleHeight < 3)aHoleHeight++;
        }
        
        for(int i=aHoleX;i<(aHoleX + aHoleWidth);i++)
        {
            for(int n=aHoleY;n<(aHoleY + aHoleHeight);n++)
            {
                mLayerHoles->Set(i, n, 1);
                
            }
        }
        
    }
    
    
    
    int aBlockCount = 2 + gRand.Get(4) + gRand.Get(6);
    
    for(int aBlockIndex = 0;aBlockIndex<aBlockCount;aBlockIndex++)
    {
        int aBlockX = gRand.Get(mGridWidth);
        int aBlockY = gRand.Get(mGridHeight);
        
        if(mLayerHoles->Get(aBlockX, aBlockY) == 0)
        {
            GameTileBlock *aBlock = new GameTileBlock();
            mTile[aBlockX][aBlockY] = aBlock;
            aBlock->SetUp(aBlockX, aBlockY, mTileWidth, mTileHeight, mOffsetX, mOffsetY);
            
        }
        
    }
    
    //mLayerHoles->Flood(0);
    
    
    for(int i=0;i<mGridWidth;i++)
    {
        for(int n=0;n<mGridHeight;n++)
        {
            //mTile[i][n] = 0;
            
            if(mTile[i][n] == 0)
            {
            if(mLayerHoles->Get(i, n))
            {
                
            }
            else
            {
                mTile[i][n] = new GameTileMatchable();
                mTile[i][n]->mMatchType = gRand.Get(mMatchTypeCount);
            }
            }
            
            
        }
    }
    
    float aContentWidth = ((float)mGridWidth) * mTileWidth;
    float aContentHeight = ((float)mGridHeight) * mTileHeight;
    
    mOffsetX = (float)((int)(((pWidth - aContentWidth) / 2.0f) + 0.5f));
    mOffsetY = (float)((int)(((pHeight - aContentHeight) / 2.0f) + 0.5f));
    
    for(int i=0;i<mGridWidth;i++)
    {
        for(int n=0;n<mGridHeight;n++)
        {
            if(mTile[i][n])
            {
                mTile[i][n]->SetUp(i, n, mTileWidth, mTileHeight, mOffsetX, mOffsetY);
            }
        }
    }
    
    
    mLayoutWidth = pWidth;
    mLayoutHeight = pHeight;
    
}

void GameBoard::Free()
{
    FreeGrid();
    
    mLayoutWidth = -1.0f;
    mLayoutHeight = -1.0f;
    
    mAnimations.Free();
    mAnimationsAdditive.Free();
}

void GameBoard::Reset()
{
    mEffectTick = 0;
    
    mEffectTwinkleCooldown[0] = 0;
    mEffectTwinkleCooldown[1] = 0;
    mEffectTwinkleCooldown[2] = 0;
    
    
    mEffectWiggleCooldown[0] = 0;
    mEffectWiggleCooldown[1] = 0;
    mEffectWiggleCooldown[2] = 0;
    
    
    for(int i=0;i<BA_LIST_COUNT;i++)
    {
        mActionTick[i] = 0;
        mActionDrawTick[i] = 0;
        mActionRequested[i] = false;
    }
    
    SwapTilesCancel();
    
}

void GameBoard::Reset(int pLevel)
{
    Reset();
    
}

bool GameBoard::IsPaused()
{
    return false;
}

int GameBoard::GetTouchGridX(float pX)
{
    int aReturn = -1;
    pX -= mOffsetX;
    
    if(pX > 0)
    {
        pX /= mTileWidth;
        if(pX < mGridWidth)
        {
            aReturn = pX;
        }
    }
    return aReturn;
}

int GameBoard::GetTouchGridY(float pY)
{
    int aReturn = -1;
    pY -= mOffsetY;
    
    if(pY > 0)
    {
        pY /= mTileHeight;
        if(pY < mGridHeight)
        {
            aReturn = pY;
        }
    }
    return aReturn;
}

float GameBoard::GetTileCenterX(int pGridX)
{
    return (float)pGridX * mTileWidth + (mTileWidth / 2.0f) + mOffsetX;
}

float GameBoard::GetTileCenterY(int pGridY)
{
    return (float)pGridY * mTileHeight + (mTileHeight / 2.0f) + mOffsetY;
}

void GameBoard::AddLayer(BoardActionLayer *pLayer)
{
    mLayerList += pLayer;
}


FString GameBoard::ActionName(int pActionType)
{
    FString aReturn = "";
    
    if(pActionType == BA_TOPPLE)aReturn = "TOPL";
    //if(pActionType == BA_MATCH)aReturn = "MACH";
    if(pActionType == BA_DESTROY)aReturn = "BUST";
    if(pActionType == BA_SWAP)aReturn = "SWAP";
    if(pActionType == BA_ZAP_ITEM)aReturn = "IT-Z";
    if(pActionType == BA_CASCADE_CHECK)aReturn = "CASC";
    if(pActionType == BA_USER_ITEM)aReturn = "IT-U";
    if(pActionType == BA_POWER_UP)aReturn = "POWR";
    if(pActionType == BA_BONUS)aReturn = "BONS";
    
    return aReturn;
}

void GameBoard::ActionActivate(int pActionType)
{
    if(mActionTick[pActionType] == 0)Log("Action On! (%s)\n", ActionName(pActionType).c());
    
    //KLUD
    //mActionTickDefault[pActionType] = 20 + gRand.Get(40);
    
    
    mActionTick[pActionType] = mActionTickDefault[pActionType];
    
    //mActionRequested[
    
    mActionDrawTick[pActionType] = 60;
    //    for(int aIndex=0;aIndex<mActionAutoRequestCount[pActionType];aIndex++)
    //    {
    //        int aActionType = mActionAutoRequest[pActionType][aIndex];
    //        if(mActionRequest[aActionType] == 0)
    //        {
    //            Log("Action Auto Request! (%s)\n", ActionName(aActionType).c());
    //            ActionRequest(aActionType);
    //        }
    //    }
}


void GameBoard::ActionRequest(int pActionType)
{
    mActionRequested[pActionType] = true;
}

void GameBoard::ActionUnrequest(int pActionType)
{
    mActionRequested[pActionType] = false;
}


bool GameBoard::SwapTiles(int pGridX1, int pGridY1, int pGridX2, int pGridY2, bool pReverse)
{
    bool aSuccess = false;
    
    GameTileMatchable *aSwapTile1 = GetTileMatchable(pGridX1, pGridY1);
    GameTileMatchable *aSwapTile2 = GetTileMatchable(pGridX2, pGridY2);
    
    mSwapTile[0] = aSwapTile1;
    mSwapTileGridX[0] = pGridX1;
    mSwapTileGridY[0] = pGridY1;
    
    mSwapTile[1] = aSwapTile2;
    mSwapTileGridX[1] = pGridX2;
    mSwapTileGridY[1] = pGridY2;
    
    if((aSwapTile1 != 0) && (aSwapTile2 != 0) && (aSwapTile1 != aSwapTile2) && (SwapTilesAllowed(pGridX1, pGridY1, pGridX2, pGridY2) == true))
    {
        ActionActivate(BA_SWAP);
        mSwap = true;
        mSwapReverse = pReverse;
        mSwapTimer = SWAP_TIME;
        aSuccess = true;
    }
    
    return aSuccess;
}

bool GameBoard::SwapTilesAllowed(int pGridX1, int pGridY1, int pGridX2, int pGridY2)
{
    bool aSuccess = false;
    
    
    
    GameTileMatchable *aSwapTile1 = GetTileMatchable(pGridX1, pGridY1);
    GameTileMatchable *aSwapTile2 = GetTileMatchable(pGridX2, pGridY2);
    
    if((aSwapTile1 != 0) && (aSwapTile2 != 0) && (aSwapTile1 != aSwapTile2))
    {
        //if(AllowSwap())
        //{
        int aDiffX = pGridX1 - pGridX2;
        int aDiffY = pGridY1 - pGridY2;
        if(((aDiffX == 1) || (aDiffX == -1)) && (aDiffY == 0))aSuccess = true;
        
        if(((aDiffY == 1) || (aDiffY == -1)) && (aDiffX == 0))aSuccess = true;
        
    }
    
    return aSuccess;
}


void GameBoard::SwapTilesCancel()
{
    GameTileMatchable *aSwapTile1 = 0;
    GameTileMatchable *aSwapTile2 = 0;
    
    if(mSwapTile[0])aSwapTile1 = GetTileMatchable(mSwapTileGridX[0], mSwapTileGridY[0]);
    if(mSwapTile[1])aSwapTile2 = GetTileMatchable(mSwapTileGridX[1], mSwapTileGridY[1]);
    
    if(aSwapTile1)aSwapTile1->AnimationDeselect();
    if(aSwapTile2)aSwapTile1->AnimationDeselect();
    
    mSwapDragStartX = 0.0f;
    mSwapDragStartY = 0.0f;
    mSwapDragData = 0;
    
    mSwapTile[0] = 0;
    mSwapTileGridX[0] = -1;
    mSwapTileGridY[0] = -1;
    
    mSwapTile[1] = 0;
    mSwapTileGridX[1] = -1;
    mSwapTileGridY[1] = -1;
    
    mSwap = false;
    mSwapReverse = false;
    mSwapWillMatch = false;
}

void GameBoard::SizeGrid(int pGridWidth, int pGridHeight)
{
    FreeGrid();
    
    mGridWidth = pGridWidth;
    mGridHeight = pGridHeight;
    
    mTile = new GameTile**[mGridWidth];
    
    for(int i=0;i<mGridWidth;i++)
    {
        mTile[i] = new GameTile*[mGridHeight];
        for(int n=0;n<mGridHeight;n++)
        {
            mTile[i][n] = 0;
        }
    }
    
    mLayerMatchType = new BoardActionLayer();AddLayer(mLayerMatchType);
    mLayerMatchCheck = new BoardActionLayer();AddLayer(mLayerMatchCheck);
    mLayerMatchFlagged = new BoardActionLayer();AddLayer(mLayerMatchFlagged);
    mLayerMatchCount = new BoardActionLayer();AddLayer(mLayerMatchCount);
    mLayerMatchStackX = new BoardActionLayer();AddLayer(mLayerMatchStackX);
    mLayerMatchStackY = new BoardActionLayer();AddLayer(mLayerMatchStackY);
    mLayerHoles = new BoardActionLayer();AddLayer(mLayerHoles);
    
    mLayerHoles->mDefaultValue = 0;
    mLayerHoles->Flood(0);
    
    
    EnumList(BoardActionLayer, aLayer, mLayerList)
    {
        aLayer->SetSize(mGridWidth, mGridHeight);
    }
}

bool GameBoard::AllowSelect()
{
    bool aReturn = true;
    
    
    
    if(IsActionActive(BA_SWAP))aReturn = false;
    if(IsActionActive(BA_DESTROY))aReturn = false;
    if(IsActionActive(BA_TOPPLE, BA_CASCADE_CHECK))aReturn = false;
    if(IsActionActive(BA_ZAP_ITEM, BA_USER_ITEM, BA_POWER_UP, BA_BONUS))aReturn = false;
    
    /*
     FString aAllowType = "";
     if(IsActionActive(BA_SWAP))aAllowType += "[SWP]";
     if(IsActionActive(BA_DESTROY))aAllowType += "[DES]";
     if(IsActionActive(BA_TOPPLE))aAllowType += "[FAL]";
     if(IsActionActive(BA_CASCADE_CHECK))aAllowType += "[CSC]";
     if(IsActionActive(BA_ZAP_ITEM))aAllowType += "[ITM_Z]";
     if(IsActionActive(BA_USER_ITEM))aAllowType += "[ITM_U]";
     if(IsActionActive(BA_POWER_UP))aAllowType += "[POW]";
     if(IsActionActive(BA_BONUS))aAllowType += "[BON]";
     
     if(aReturn)Log("Click - Allowing [%s]\n", aAllowType.c());
     else Log("Click - Disallowing [%s]\n", aAllowType.c());
     */
    
    return aReturn;
}

//bool GameBoard::AllowSwap()
//{
//    bool aReturn = true;
//
//    //if(IsActionActive())aReturn = false;
//    if(IsActionActive(BA_SWAP, BA_DESTROY, BA_TOPPLE, BA_CASCADE_CHECK))aReturn = false;
//    if(IsActionActive(BA_ZAP_ITEM, BA_USER_ITEM, BA_POWER_UP, BA_BONUS))aReturn = false;
//
//    return aReturn;
//}

bool GameBoard::AllowDestroy()
{
    bool aReturn = true;
    
    //if(IsActionActive(BA_SWAP, BA_DESTROY, BA_TOPPLE, BA_CASCADE_CHECK))aReturn = false;
    if(IsActionActive(BA_SWAP, BA_TOPPLE, BA_CASCADE_CHECK))aReturn = false;
    
    return aReturn;
}

bool GameBoard::AllowCascadeCheck()
{
    bool aReturn = true;
    if(IsActionActive(BA_SWAP, BA_DESTROY, BA_TOPPLE))aReturn = false;
    return aReturn;
}

bool GameBoard::AllowTopple()
{
    bool aReturn = true;
    if(IsActionActive(BA_SWAP, BA_DESTROY))aReturn = false;
    return aReturn;
}

bool GameBoard::AllowItemZap()
{
    bool aReturn = true;
    
    if(IsActionActive(BA_SWAP))aReturn = false;
    if(IsActionActive(BA_DESTROY))aReturn = false;
    if(IsActionActive(BA_TOPPLE))aReturn = false;
    //if(IsActionActive(BA_CASCADE_CHECK))aReturn = false;
    //if(IsActionActive(BA_ZAP_ITEM))aReturn = false;
    //if(IsActionActive(BA_USER_ITEM))aReturn = false;
    if(IsActionActive(BA_POWER_UP))aReturn = false;
    //if(IsActionActive(BA_BONUS))aReturn = false;
    
    return aReturn;
}

bool GameBoard::AllowItemUser()
{
    bool aReturn = true;
    
    if(IsActionActive(BA_SWAP))aReturn = false;
    if(IsActionActive(BA_DESTROY))aReturn = false;
    if(IsActionActive(BA_TOPPLE))aReturn = false;
    if(IsActionActive(BA_CASCADE_CHECK))aReturn = false;
    if(IsActionActive(BA_ZAP_ITEM))aReturn = false;
    if(IsActionActive(BA_POWER_UP))aReturn = false;
    if(IsActionActive(BA_BONUS))aReturn = false;
    
    return aReturn;
}


bool GameBoard::HandleMatches()
{
    Log("--HandleMatches()\n");
    bool aReturn = false;
    BoardMatch *aMatch = 0;
    GameTileMatchable *aMatchTile = 0;
    
    int aMatchX = -1;
    int aMatchY = -1;
    
    for(int aMatchIndex=0;aMatchIndex<mMatchCount;aMatchIndex++)
    {
        aMatch = mMatch[aMatchIndex];
        
        for(int i=0;i<aMatch->mCount;i++)
        {
            aMatchX = aMatch->mX[i];
            aMatchY = aMatch->mY[i];
            
            aMatchTile = GetTileMatchable(aMatchX, aMatchY);
            
            if(aMatchTile)
            {
                aReturn = true;
                aMatchTile->Destroy(0, 180);
                aMatchTile->AnimationMatch();
            }
        }
    }
    
    if(aReturn)
    {
        ActionActivate(BA_DESTROY);
        ActionRequest(BA_DESTROY);
    }
    
    return aReturn;
}

bool GameBoard::HandleZapItem()
{
    ActionUnrequest(BA_ZAP_ITEM);
    
    bool aReturn = false;
    
    return aReturn;
}

bool GameBoard::HandleDestroy()
{
    //Log("--HandleDestroy()\n");
    
    ActionUnrequest(BA_DESTROY);
    
    bool aReturn = false;
    
    GameTile *aTile = 0;
    
    for(int n=mGridHeight-1;n>=0;n--)
    {
        for(int i=0;i<mGridWidth;i++)
        {
            aTile = mTile[i][n];
            if(aTile)
            {
                if(aTile->mDestroyed == true)
                {
                    
                    aReturn = true;
                    DeleteTile(i, n);
                }
            }
        }
    }
    
    //if(aReturn)ActionActivate(BA_DESTROY);
    
    return aReturn;
}

bool GameBoard::HandleCascadeCheck()
{
    ActionUnrequest(BA_CASCADE_CHECK);
    
    //Log("--HandleCascadeCheck()\n");
    
    bool aReturn = false;
    
    int aCount = 0;
    
    GameTile *aTile = 0;
    GameTileMatchable *aMatchTile = 0;
    
    int *aCheckX = mLayerMatchStackX->mDataBase;
    int *aCheckY = mLayerMatchStackY->mDataBase;
    
    
    
    for(int i=0;i<mGridWidth;i++)
    {
        for(int n=0;n<mGridHeight;n++)
        {
            aTile = GetTile(i, n);
            
            if(aTile)
            {
                if(aTile->mCascadeCheck == true)
                {
                    aTile->mCascadeCheck = false;
                    
                    if(aTile->mTileType == GAME_TILE_TYPE_MATCHABLE)
                    {
                        aCheckX[aCount] = i;
                        aCheckY[aCount] = n;
                        
                        aCount++;
                    }
                }
                
            }
        }
    }
    
    int aGridX = 0;
    int aGridY = 0;
    
    MatchComputeSetup();
    
    for(int aIndex=0;aIndex<aCount;aIndex++)
    {
        aGridX = aCheckX[aIndex];
        aGridY = aCheckY[aIndex];
        
        aMatchTile = GetTileMatchable(aGridX, aGridY);
        
        if(aMatchTile)
        {
            MatchCompute(aGridX, aGridY);
        }
        
    }
    
    if(mMatchCount > 0)
    {
        HandleMatches();
        //ActionRequest(BA_MATCH);
    }
    
    //HandleCascadeCheck(
    
    return aReturn;
}


void GameBoard::MatchComputeSetup()
{
    MatchComputeSetup(0, 0, mGridWidth - 1, mGridHeight - 1);
    
}

void GameBoard::MatchComputeSetup(int pGridStartX, int pGridStartY, int pGridEndX, int pGridEndY)
{
    mMatchCount = 0;
    
    if(pGridStartX >= mGridWidth)pGridStartX = (mGridWidth - 1);
    if(pGridEndX >= mGridWidth)pGridEndX = (mGridWidth - 1);
    if(pGridStartY >= mGridHeight)pGridStartY = (mGridHeight - 1);
    if(pGridEndY >= mGridHeight)pGridEndY = (mGridHeight - 1);
    if(pGridStartX < 0)pGridStartX = 0;
    if(pGridStartY < 0)pGridStartY = 0;
    if((pGridEndX < pGridStartX) || (pGridEndY < pGridStartY) || (mLayerMatchType == 0))return;
    
    GameTile *aTile = 0;
    int aType = -1;
    
    for(int i=pGridStartX;i<=pGridEndX;i++)
    {
        for(int n=pGridStartY;n<=pGridEndY;n++)
        {
            aTile = mTile[i][n];
            aType = -1;
            
            if(aTile)
            {
                if(aTile->mDestroyed == false)aType = aTile->mMatchType;
            }
            
            mLayerMatchType->Set(i, n, aType);
            mLayerMatchCheck->Set(i, n, 0);
            mLayerMatchFlagged->Set(i, n, 0);
        }
    }
}

bool GameBoard::MatchCompute(int pGridX, int pGridY)
{
    bool aReturn = false;
    
    if(MatchComputeH(pGridX, pGridY))aReturn = true;
    if(MatchComputeV(pGridX, pGridY))aReturn = true;
    
    return aReturn;
}

bool GameBoard::MatchComputeH(int pGridX, int pGridY)
{
    bool aReturn = false;
    if(OnGrid(pGridX, pGridY))
    {
        if(MatchExistsH(pGridX, pGridY) == false)
        {
            int **aMatchType = mLayerMatchType->mData;
            int aType = aMatchType[pGridX][pGridY];
            
            if(aType != -1)
            {
                int aCount = 1;
                for(int i=pGridX+1;i<mGridWidth;i++)
                {
                    if(aMatchType[i][pGridY] == aType)aCount++;
                    else break;
                }
                for(int i=pGridX-1;i>=0;i--)
                {
                    if(aMatchType[i][pGridY] == aType)aCount++;
                    else break;
                }
                
                if(aCount >= mMatchRequiredCount)
                {
                    aReturn = true;
                    
                    MatchListExpand();
                    BoardMatch *aMatch = mMatch[mMatchCount];
                    mMatchCount++;
                    aMatch->Reset();
                    aMatch->Add(pGridX, pGridY);
                    
                    for(int i=pGridX+1;i<mGridWidth;i++)
                    {
                        if(aMatchType[i][pGridY] == aType)aMatch->Add(i, pGridY);
                        else break;
                    }
                    for(int i=pGridX-1;i>=0;i--)
                    {
                        if(aMatchType[i][pGridY] == aType)aMatch->Add(i, pGridY);
                        else break;
                    }
                    
                    aMatch->Compute(0, 0, mGridWidth - 1, mGridHeight - 1);
                    
                    PrintMatchContent(aMatch);
                }
            }
        }
    }
    
    return aReturn;
}

bool GameBoard::MatchComputeV(int pGridX, int pGridY)
{
    bool aReturn = false;
    if(OnGrid(pGridX, pGridY))
    {
        if(MatchExistsH(pGridX, pGridY) == false)
        {
            int **aMatchType = mLayerMatchType->mData;
            int aType = aMatchType[pGridX][pGridY];
            
            if(aType != -1)
            {
                int aCount = 1;
                for(int n=pGridY+1;n<mGridHeight;n++)
                {
                    if(aMatchType[pGridX][n] == aType)aCount++;
                    else break;
                }
                for(int n=pGridY-1;n>=0;n--)
                {
                    if(aMatchType[pGridX][n] == aType)aCount++;
                    else break;
                }
                
                if(aCount >= mMatchRequiredCount)
                {
                    aReturn = true;
                    
                    MatchListExpand();
                    BoardMatch *aMatch = mMatch[mMatchCount];
                    mMatchCount++;
                    aMatch->Reset();
                    aMatch->Add(pGridX, pGridY);
                    
                    for(int n=pGridY+1;n<mGridHeight;n++)
                    {
                        if(aMatchType[pGridX][n] == aType)aMatch->Add(pGridX, n);
                        else break;
                    }
                    for(int n=pGridY-1;n>=0;n--)
                    {
                        if(aMatchType[pGridX][n] == aType)aMatch->Add(pGridX, n);
                        else break;
                    }
                    
                    aMatch->Compute(0, 0, mGridWidth - 1, mGridHeight - 1);
                    
                    PrintMatchContent(aMatch);
                }
            }
        }
    }
    
    return aReturn;
}

bool GameBoard::MatchExistsH(int pGridX, int pGridY)
{
    bool aReturn = false;
    
    if(OnGrid(pGridX, pGridY))
    {
        
    }
    
    return aReturn;
}

bool GameBoard::MatchExistsV(int pGridX, int pGridY)
{
    bool aReturn = false;
    
    if(OnGrid(pGridX, pGridY))
    {
        
    }
    
    return aReturn;
}

void GameBoard::MatchListExpand()
{
    if(mMatchCount >= mMatchSize)
    {
        int aMatchSize = mMatchCount + (mMatchCount / 2) + 1;
        
        BoardMatch **aMatchNew = new BoardMatch*[aMatchSize];
        for(int k=0;k<mMatchSize;k++)aMatchNew[k] = mMatch[k];
        delete [] mMatch;
        mMatch = aMatchNew;
        for(int k=mMatchSize;k<aMatchSize;k++)mMatch[k] = new BoardMatch();
        mMatchSize = aMatchSize;
    }
}

void GameBoard::MatchListSort()
{
    BoardMatch *aMatchSortKey = 0;
    BoardMatch *aMatchHold = 0;
    
    int aCheck = 0;
    
    for(int aStart = 1;aStart<mMatchCount;aStart++)
    {
        aMatchSortKey = mMatch[aStart];
        aCheck=aStart-1;
        while(aCheck >= 0 && (mMatch[aCheck]->mCount) < aMatchSortKey->mCount)
        {
            mMatch[aCheck+1] = mMatch[aCheck];
            aCheck--;
        }
        mMatch[aCheck+1]=aMatchSortKey;
    }
    
    int aMatchCountTies = 1;
    
    int aIndex = 0;
    int aSeek = 0;
    bool aContinue = true;
    
    int aRand = 0;
    
    while(aIndex < mMatchCount)
    {
        aMatchCountTies = 1;
        aSeek = aIndex + 1;
        aContinue = true;
        while((aSeek < mMatchCount) && (aContinue == true))
        {
            if(mMatch[aSeek]->mCount != mMatch[aIndex]->mCount)aContinue = false;
            else aMatchCountTies++;
            aSeek++;
        }
        
        if(aMatchCountTies > 1)
        {
            for(int i=0;i<aMatchCountTies;i++)
            {
                aRand = gRand.Get(aMatchCountTies);
                
                if(aRand != i)
                {
                    aMatchHold = mMatch[aIndex + i];
                    mMatch[aIndex + i] = mMatch[aIndex + aRand];
                    mMatch[aIndex + aRand] = aMatchHold;
                }
            }
        }
        
        aIndex += aMatchCountTies;
    }
}

void GameBoard::PrintMatchContent(BoardMatch *pMatch)
{
    FString aMatchContent = "{";
    
    FString aMatchGrid = "{";
    
    for(int i=0;i<pMatch->mCount;i++)
    {
        GameTile *aTile = GetTile(pMatch->mX[i], pMatch->mY[i]);
        
        if(aTile)
        {
            aMatchContent += FString("(") + FString(aTile->mTileType) + FString(")");
            aMatchGrid += FString("[") + FString(aTile->mGridX) + FString(", ") + FString(aTile->mGridY) + FString("]");
        }
        
        if(i < (pMatch->mCount - 1))
        {
            aMatchContent += ", ";
            aMatchGrid += ", ";
        }
        
    }
    
    aMatchContent += "}";
    aMatchGrid += "}";
    
    int aIndex = -1;
    for(int i=0;i<mMatchCount;i++)
    {
        if(mMatch[i] == pMatch)aIndex = i;
    }
    
    FString aDir = "";
    if(pMatch->mHorizontal)
    {
        if(pMatch->mVertical)
        {
            aDir = "Hor+Ver";
        }
        else
        {
            aDir = "Hor";
        }
    }
    else if(pMatch->mVertical)
    {
        aDir = "Ver";
    }
    
    Log("Match[%d] - %d, {%s}\nContent - %s\nGrid - %s\n\n", aIndex, pMatch->mCount, aDir.c(), aMatchContent.c(), aMatchGrid.c());
}


void GameBoard::ResizeGrid(int pGridWidth, int pGridHeight)
{
    if(pGridWidth <= 0 || pGridHeight <= 0)
    {
        FreeGrid();
    }
    else
    {
        GameTile ***aOld = mTile;
        
        int aWidthOld = mGridWidth;
        int aHeightOld = mGridHeight;
        
        mGridWidth = pGridWidth;
        mGridHeight = pGridHeight;
        
        mTile = new GameTile**[mGridWidth];
        
        for(int i=0;i<mGridWidth;i++)
        {
            mTile[i] = new GameTile*[mGridHeight];
            for(int n=0;n<mGridHeight;n++)
            {
                mTile[i][n] = 0;
            }
        }
        
        int aCopyWidth = aWidthOld;
        if(mGridWidth < aCopyWidth)aCopyWidth = mGridWidth;
        
        int aCopyHeight = aHeightOld;
        if(mGridHeight < aCopyHeight)aCopyHeight = mGridHeight;
        
        for(int i=0;i<aCopyWidth;i++)
        {
            for(int n=0;n<aCopyHeight;n++)
            {
                mTile[i][n] = aOld[i][n];
            }
        }
        
        for(int i=0;i<aWidthOld;i++)
        {
            for(int n=0;n<aHeightOld;n++)
            {
                
            }
            delete [] aOld[i];
        }
        delete [] aOld;
        
        EnumList(BoardActionLayer, aLayer, mLayerList)
        {
            aLayer->Resize(mGridWidth, mGridHeight);
        }
    }
}

void GameBoard::FreeGrid()
{
    Reset();
    
    FList aKillTileList;
    
    EnumList(GameTile, aTile, mTilesDeletedThisUpdate)
    {
        if(aKillTileList.Exists(aTile) == false)aKillTileList.Add(aTile);
    }
    
    mTilesDeletedThisUpdate.Clear();
    
    EnumList(GameTile, aTile, mTilesDeleted)
    {
        if(aKillTileList.Exists(aTile) == false)aKillTileList.Add(aTile);
    }
    
    mTilesDeleted.Clear();
    
    EnumList(GameTile, aTile, aKillTileList)
    {
        RemoveTile(aTile->mGridX, aTile->mGridY);
        delete aTile;
    }
    
    aKillTileList.Clear();
    
    if(mTile)
    {
        for(int i=0;i<mGridWidth;i++)
        {
            for(int n=0;n<mGridHeight;n++)
            {
                DeleteTile(i, n);
                
                mTile[i][n] = 0;
            }
            delete [] mTile[i];
        }
        
        delete [] mTile;
        mTile = 0;
    }
    
    for(int i=0;i<mMatchCount;i++)
    {
        delete mMatch[i];
        mMatch[i] = 0;
    }
    
    delete [] mMatch;
    
    mMatch = 0;
    mMatchCount = 0;
    mMatchSize = 0;
    
    mGridWidth = 0;
    mGridHeight = 0;
    
    
    EnumList(BoardActionLayer, aLayer, mLayerList)delete aLayer;
    mLayerList.Clear();
    
    
    mLayerMatchType = 0;
    mLayerMatchCheck = 0;
    mLayerMatchFlagged = 0;
    mLayerMatchCount = 0;
    mLayerMatchStackX = 0;
    mLayerMatchStackY = 0;
    mLayerHoles = 0;
}

void GameBoard::PadGrid(int pBlockCountTop, int pBlockCountBottom)
{
    int aNewHeight = mGridHeight + (pBlockCountTop + pBlockCountBottom);
    
    if(mGridWidth <= 0)mGridWidth = 7;
    
    if((aNewHeight <= 0) || (mGridHeight <= 0))
    {
        FreeGrid();
    }
    else
    {
        if(pBlockCountTop < 0)
        {
            int aCount = (-pBlockCountTop);
            for(int n=0;n<aCount;n++)
            {
                for(int i=0;i<mGridWidth;i++)
                {
                    DeleteTile(i, n);
                }
            }
        }
        
        if(pBlockCountBottom < 0)
        {
            for(int n=(mGridHeight+pBlockCountBottom);n<mGridHeight;n++)
            {
                for(int i=0;i<mGridWidth;i++)
                {
                    DeleteTile(i, n);
                }
            }
        }
        
        GameTile ***aNewTile = new GameTile**[mGridWidth];
        
        for(int i=0;i<mGridWidth;i++)
        {
            aNewTile[i] = new GameTile*[aNewHeight];
            for(int n=0;n<aNewHeight;n++)
            {
                aNewTile[i][n] = 0;
            }
        }
        
        int aCopyY = (-pBlockCountTop);
        for(int n=0;n<aNewHeight;n++)
        {
            if((aCopyY < mGridHeight) && (aCopyY >= 0))
            {
                for(int i=0;i<mGridWidth;i++)
                {
                    aNewTile[i][n] = mTile[i][aCopyY];
                    mTile[i][aCopyY] = 0;
                }
            }
            aCopyY++;
        }
        
        SizeGrid(mGridWidth, aNewHeight);
        
        for(int i=0;i<mGridWidth;i++)
        {
            for(int n=0;n<aNewHeight;n++)
            {
                GameTile *aTile = aNewTile[i][n];
                if(aTile)
                {
                    mTile[i][n] = aTile;
                    aTile->SetUp(i, n, mTileWidth, mTileHeight, mOffsetX, mOffsetY);
                }
                else
                {
                    mTile[i][n] = 0;
                }
            }
            delete [] aNewTile[i];
        }
        delete [] aNewTile;
    }
}

bool GameBoard::OnGrid(int pGridX, int pGridY)
{
    bool aReturn = ((pGridX >= 0) && (pGridY >= 0) && (pGridX < mGridWidth) && (pGridY < mGridHeight));
    
    return aReturn;
}

bool GameBoard::CanTopple(int pGridX, int pGridY)
{
    bool aReturn = false;
    
    
    return aReturn;
}

bool GameBoard::CanLand(int pGridX, int pGridY)
{
    bool aReturn = false;
    
    
    return aReturn;
}

bool GameBoard::CanSetTile(int pGridX, int pGridY, GameTile *pTile)
{
    bool aReturn = false;
    
    return aReturn;
}

bool GameBoard::SetTile(int pGridX, int pGridY, GameTile *pTile)
{
    if(OnGrid(pGridX, pGridY))
    {
        if(pTile->mMulti)
        {
            GameTile *aPart = 0;
            
            bool aIllegal = false;
            
            for(int i=0;i<pTile->mMultiPartCount;i++)
            {
                aPart = pTile->mMultiPartList[i];
                
                if(OnGrid(aPart->mMultiPartGridOffsetX + pGridX, aPart->mMultiPartGridOffsetY + pGridY) == false)
                {
                    aIllegal = true;
                }
            }
            
            if(aIllegal == true)
            {
                return false;
            }
            
            aPart = 0;
            
            for(int i=0;i<pTile->mMultiPartCount;i++)
            {
                aPart = pTile->mMultiPartList[i];
                
                DeleteTile(aPart->mMultiPartGridOffsetX + pGridX, aPart->mMultiPartGridOffsetY + pGridY);
            }
        }
        
        DeleteTile(pGridX, pGridY);
        
        if(pTile->mMulti)
        {
            GameTile *aPart = 0;
            
            for(int i=0;i<pTile->mMultiPartCount;i++)
            {
                aPart = pTile->mMultiPartList[i];
                
                aPart->mGridX = pGridX + aPart->mMultiPartGridOffsetX;
                aPart->mGridY = pGridY + aPart->mMultiPartGridOffsetY;
                
                mTile[aPart->mGridX][aPart->mGridY] = aPart;
            }
            
            mTile[pGridX][pGridY] = pTile;
            pTile->SetUp(pGridX, pGridY, mTileWidth, mTileHeight, mOffsetX, mOffsetY);
        }
        else
        {
            mTile[pGridX][pGridY] = pTile;
            pTile->SetUp(pGridX, pGridY, mTileWidth, mTileHeight, mOffsetX, mOffsetY);
        }
    }
    else
    {
        return false;
    }
    
    return true;
}

GameTile *GameBoard::GetTile(int pGridX, int pGridY)
{
    GameTile *aReturn = 0;
    
    if((pGridX >= 0) && (pGridY >= 0) && (pGridX < mGridWidth) && (pGridY < mGridHeight))
    {
        aReturn = mTile[pGridX][pGridY];
        
        if(aReturn)
        {
            if((aReturn->mMultiPart == true) && (aReturn->mMultiPartParent != 0))
            {
                aReturn = aReturn->mMultiPartParent;
            }
        }
    }
    
    return aReturn;
}

GameTile *GameBoard::GetTile(int pGridX, int pGridY, int pTileType)
{
    GameTile *aReturn = GetTile(pGridX, pGridY);
    
    if(aReturn != 0)
    {
        if(aReturn->mTileType != pTileType)
        {
            aReturn = 0;
        }
    }
    
    return aReturn;
}

GameTileMatchable *GameBoard::GetRandomMatchableTile()
{
    int aCount = 0;
    GameTileMatchable *aMatchTile = 0;
    int *aCheckX = mLayerMatchStackX->mDataBase;
    int *aCheckY = mLayerMatchStackY->mDataBase;
    for(int i=0;i<mGridWidth;i++)
    {
        for(int n=0;n<mGridHeight;n++)
        {
            aMatchTile = GetTileMatchable(i, n);
            aCheckX[aCount] = i;
            aCheckY[aCount] = n;
            aCount++;
        }
    }
    aMatchTile = 0;
    if(aCount > 0)
    {
        int aIndex = gRand.Get(aCount);
        aMatchTile = GetTileMatchable(aCheckX[aIndex], aCheckY[aIndex]);
    }
    return aMatchTile;
}

GameTile *GameBoard::RemoveTile(int pGridX, int pGridY)
{
    GameTile *aReturn = GetTile(pGridX, pGridY);
    
    if(aReturn)
    {
        if(aReturn->mMulti)
        {
            GameTile *aPart = 0;
            int aGridX = 0;
            int aGridY = 0;
            
            for(int i=0;i<aReturn->mMultiPartCount;i++)
            {
                aPart = aReturn->mMultiPartList[i];
                
                aGridX = aPart->mGridX;
                aGridY = aPart->mGridY;
                
                mTile[aGridX][aGridY] = 0;
            }
            
            mTile[aReturn->mGridX][aReturn->mGridY] = 0;
        }
        else
        {
            mTile[pGridX][pGridY] = 0;
        }
    }
    
    return aReturn;
}

void GameBoard::DeleteTile(int pGridX, int pGridY)
{
    GameTile *aTile = RemoveTile(pGridX, pGridY);
    
    if(aTile != 0)
    {
        aTile->mTimerDelete = 20;
        mTilesDeleted.Add(aTile);
    }
}

bool GameBoard::Topple()
{
    bool aReturn = false;
    for(int i=0;i<mGridWidth;i++)
    {
        GameTile *aTile = 0;
        
        int aShiftAmount = 0;
        
        for(int n=mGridHeight-1;n>=0;n--)
        {
            aTile = mTile[i][n];
            
            if(aTile)
            {
                if(aShiftAmount > 0)
                {
                    mTile[i][n] = 0;
                    mTile[i][n + aShiftAmount] = aTile;
                    aTile->FallTo(n + aShiftAmount, mOffsetY);
                }
            }
            else
            {
                aShiftAmount++;
            }
            
            if(aShiftAmount > 0)
            {
                aReturn = true;
            }
        }
    }
    
    if(aReturn == true)
    {
        ActionActivate(BA_TOPPLE);
        ActionRequest(BA_CASCADE_CHECK);
    }
    
    if(FillHoles())
    {
        aReturn = true;
        RandomizeNewTiles();
    }
    
    return aReturn;
}

bool GameBoard::FillHoles()
{
    bool aReturn = false;
    
    GameTileMatchable *aMatchTile = 0;
    
    for(int i=0;i<mGridWidth;i++)
    {
        int aShiftAmount = 0;
        
        aShiftAmount = 0;
        
        int aFillCount = 0;
        
        for(int n=0;n<mGridHeight;n++)
        {
            if(mTile[i][n] == 0)
            {
                aFillCount++;
            }
            else
            {
                break;
            }
        }
        
        int aSpawnStart = -1;
        int aSpawnTarget = (aFillCount - 1);
        
        if(aFillCount > 0)
        {
            aReturn = true;
            for(int k=0;k<aFillCount;k++)
            {
                aMatchTile = new GameTileMatchable();
                aMatchTile->mMatchType = gRand.Get(mMatchTypeCount);
                aMatchTile->mFreshSpawn = true;
                
                SetTile(i, aSpawnTarget, aMatchTile);
                
                aMatchTile->mCenterY = GetTileCenterY(aSpawnStart) - 140.0f;
                aMatchTile->FallTo(aSpawnTarget, mOffsetY);
                
                aSpawnTarget--;
                aSpawnStart--;
                
                ActionActivate(BA_TOPPLE);
                ActionRequest(BA_CASCADE_CHECK);
            }
        }
    }
    
    return aReturn;
}


void GameBoard::RandomizeNewTiles()
{
    //TODO: Add check for making sure there is an existing match.
    
    GameTile *aTile = 0;
    GameTileMatchable *aTileMatchable = 0;
    
    FList aList;
    
    for(int i=0;i<mGridWidth;i++)
    {
        for(int n=0;n<mGridHeight;n++)
        {
            aTile = GetTile(i, n);
            aTileMatchable = GetTileMatchable(i, n);
            
            if(aTileMatchable)
            {
                if((aTileMatchable->mFreshSpawn == true) && (aTileMatchable->mKill == 0))
                {
                    aList.Add(aTileMatchable);
                }
            }
        }
    }
    
    GameTileMatchable *aRandomTile = (GameTileMatchable *)(aList.FetchRandom());
    
    if(gRand.Get(4) == 0)
    {
        if(aRandomTile)
        {
            //aRandomTile->GenerateComboTrigger();
        }
    }
    
    for(int i=0;i<mGridWidth;i++)
    {
        for(int n=0;n<mGridHeight;n++)
        {
            aTile = GetTile(i, n);
            
            if(aTile != 0)
            {
                aTile->mFreshSpawn = false;
            }
        }
    }
    
}

void GameBoard::DrawTiles()
{
    GameTile *aTile = 0;
    
    DrawTransform();
    
    Graphics::SetColor();
    for(int n=mGridHeight-1;n>=0;n--)
    {
        for(int i=0;i<mGridWidth;i++)
        {
            aTile = mTile[i][n];
            
            if((aTile != 0) && (aTile != mSwapTile[0]) && (aTile != mSwapTile[1]))
            {
                aTile->Draw();
            }
        }
    }
    Graphics::SetColor();Graphics::BlendSetAdditive();
    for(int n=mGridHeight-1;n>=0;n--)
    {
        for(int i=0;i<mGridWidth;i++)
        {
            aTile = mTile[i][n];
            
            if((aTile != 0) && (aTile != mSwapTile[0]) && (aTile != mSwapTile[1]))
            {
                aTile->DrawEffect();
            }
        }
    }
    Graphics::SetColor();Graphics::BlendSetAlpha();
    for(int n=mGridHeight-1;n>=0;n--)
    {
        for(int i=0;i<mGridWidth;i++)
        {
            aTile = mTile[i][n];
            if((aTile != 0) && (aTile != mSwapTile[0]) && (aTile != mSwapTile[1]))
            {
                aTile->DrawTop();
            }
        }
    }
    
    Graphics::SetColor();Graphics::BlendSetAlpha();
    for(int n=mGridHeight-1;n>=0;n--)
    {
        for(int i=0;i<mGridWidth;i++)
        {
            aTile = mTile[i][n];
            if((aTile != 0) && (aTile != mSwapTile[0]) && (aTile != mSwapTile[1]))
            {
                aTile->DrawEffectTop();
            }
        }
    }
    
    if(mSwapTile[0] != 0)mSwapTile[0]->Draw();
    if(mSwapTile[1] != 0)mSwapTile[1]->Draw();
    if(mSwapTile[0] != 0)mSwapTile[0]->DrawEffect();
    if(mSwapTile[1] != 0)mSwapTile[1]->DrawEffect();
    if(mSwapTile[0] != 0)mSwapTile[0]->DrawTop();
    if(mSwapTile[1] != 0)mSwapTile[1]->DrawTop();
    if(mSwapTile[0] != 0)mSwapTile[0]->DrawEffectTop();
    if(mSwapTile[1] != 0)mSwapTile[1]->DrawEffectTop();
}



























