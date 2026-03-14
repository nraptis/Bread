
#ifndef GAME_BOARD_H
#define GAME_BOARD_H

#define SWAP_TIME 18

#define BA_LIST_COUNT 12
#define BA_LIST_AUTO_COUNT 4


#define BA_SWAP 0
//#define BA_MATCH 1
#define BA_DESTROY 2
#define BA_TOPPLE 3
#define BA_CASCADE_CHECK 4
#define BA_ZAP_ITEM 5
#define BA_USER_ITEM 6
#define BA_POWER_UP 7
#define BA_BONUS 8


#include "MainApp.h"
#include "BoardMatch.h"
#include "BoardActionLayer.h"

#include "GameTile.h"
#include "GameTileMatchable.h"
#include "GameTileBlock.h"

class GameBoard : public FView
{
public:
    
    GameBoard();
    virtual ~GameBoard();
    
    
    
    virtual void                            Update();
    virtual void                            Draw();
    void                                    DrawTiles();
    
    virtual void                            TouchDown(float pX, float pY, void *pData);
    virtual void                            TouchMove(float pX, float pY, void *pData);
    virtual void                            TouchUp(float pX, float pY, void *pData);
    
    virtual void                            TouchFlush();
    
    void                                    LayoutContent(float pX, float pY, float pWidth, float pHeight);
    float                                   mLayoutWidth;
    float                                   mLayoutHeight;
    
    void                                    Free();
    void                                    FreeGrid();
    
    void                                    Reset();
    void                                    Reset(int pLevel);
    
    
    bool                                    IsPaused();
    
    FObjectList                             mAnimations;
    FObjectList                             mAnimationsAdditive;
    
    float                                   mTileWidth;
    float                                   mTileHeight;
    
    int                                     mMatchTypeCount;
    
    float                                   mOffsetX;
    float                                   mOffsetY;
    
    int                                     GetTouchGridX(float pX);
    int                                     GetTouchGridY(float pY);
    
    float                                   GetTileCenterX(int pGridX);
    float                                   GetTileCenterY(int pGridY);
    
    bool                                    AllowSelect();
    //bool                                    AllowSwap();
    bool                                    AllowDestroy();
    bool                                    AllowTopple();
    bool                                    AllowItemZap();
    bool                                    AllowItemUser();
    bool                                    AllowCascadeCheck();
    
    
    bool                                    HandleMatches();
    bool                                    HandleDestroy();
    bool                                    HandleZapItem();
    bool                                    HandleCascadeCheck();
    
    
    bool                                    mSwap;
    bool                                    mSwapReverse;
    int                                     mSwapTimer;
    
    bool                                    mSwapWillMatch;
    
    GameTileMatchable                       *mSwapTile[2];
    int                                     mSwapTileGridX[2];
    float                                   mSwapTileGridY[2];
    
    float                                   mSwapDragStartX;
    float                                   mSwapDragStartY;
    void                                    *mSwapDragData;
    
    
    bool                                    SwapTiles(int pGridX1, int pGridY1, int pGridX2, int pGridY2, bool pReverse);
    bool                                    SwapTilesAllowed(int pGridX1, int pGridY1, int pGridX2, int pGridY2);
    void                                    SwapTilesCancel();
    
    
    //mSwappingReverse
    
    FList                                   mLayerList;
    virtual void                            AddLayer(BoardActionLayer *pLayer);
    
    FList                                   mTilesDeleted;
    FList                                   mTilesDeletedThisUpdate;
    
    virtual void                            PrintMatchContent(BoardMatch *pMatch);
    
    //Some default layers that the boards will all have.. which, also, we need for here.
    BoardActionLayer                        *mLayerMatchType;
    BoardActionLayer                        *mLayerMatchCheck;
    BoardActionLayer                        *mLayerMatchFlagged;
    BoardActionLayer                        *mLayerMatchCount;
    BoardActionLayer                        *mLayerMatchStackX;
    BoardActionLayer                        *mLayerMatchStackY;
    
    BoardActionLayer                        *mLayerHoles;
    
    void                                    MatchComputeSetup();
    void                                    MatchComputeSetup(int pGridStartX, int pGridStartY, int pGridEndX, int pGridEndY);
    
    
    //Assuming we already did setup..!
    
    bool                                    MatchCompute(int pGridX, int pGridY);
    bool                                    MatchComputeH(int pGridX, int pGridY);
    bool                                    MatchComputeV(int pGridX, int pGridY);
    
    bool                                    MatchExistsH(int pGridX, int pGridY);
    bool                                    MatchExistsV(int pGridX, int pGridY);
    
    //void                                    MatchComputeAll();
    //void                                    MatchComputeAll(int pGridStartY, int pGridEndY);
    //void                                    MatchComputeAll(int pGridStartX, int pGridStartY, int pGridEndX, int pGridEndY);
    
    //void                                    MatchLayerSetup();
    //void                                    MatchLayerSetup(int pGridStartX, int pGridStartY, int pGridEndX, int pGridEndY);
    
    void                                    MatchListExpand();
    void                                    MatchListSort();
    
    
    BoardMatch                              **mMatch;
    int                                     mMatchCount;
    int                                     mMatchSize;
    
    int                                     mMatchRequiredCount;
    
    bool                                    mMatchAllowDiagonal;
    
    int                                     mActionTick[BA_LIST_COUNT];
    int                                     mActionDrawTick[BA_LIST_COUNT];
    int                                     mActionTickDefault[BA_LIST_COUNT];
    bool                                    mActionRequested[BA_LIST_COUNT];
    
    //int                                     mActionAutoRequest[BA_LIST_COUNT][BA_LIST_AUTO_COUNT];
    //int                                     mActionAutoRequestCount[BA_LIST_COUNT];
    
    
    FString                                 ActionName(int pActionType);
    void                                    ActionActivate(int pActionType);
    void                                    ActionRequest(int pActionType);
    void                                    ActionUnrequest(int pActionType);
    //void                                    ActionAutoRequest(int pActionType, int pRequestActionType);
    
    //inline bool                             IsActionOn(int pActionType){return (mActionTick[pActionType] <= 0);}
    
    //inline bool                             On(int pActionType){return ((mActionTick[pActionType] > 0) || (mActionRequest[pActionType] > 0));}
    inline bool                             IsActionActive(int pActionType){return (mActionTick[pActionType] > 0);}
    //inline bool                             Off(int pActionType){return ((mActionTick[pActionType] <= 0) && (mActionRequest[pActionType] <= 0));}
    
    inline bool                             IsActionActive(int pActionType1, int pActionType2){return (IsActionActive(pActionType1) || IsActionActive(pActionType2));}
    inline bool                             IsActionActive(int pActionType1, int pActionType2, int pActionType3){return (IsActionActive(pActionType1) || IsActionActive(pActionType2) || IsActionActive(pActionType3));}
    inline bool                             IsActionActive(int pActionType1, int pActionType2, int pActionType3, int pActionType4){return (IsActionActive(pActionType1) || IsActionActive(pActionType2) || IsActionActive(pActionType3) || IsActionActive(pActionType4));}
    
    
    inline bool                             IsActionRequested(int pActionType){return mActionRequested[pActionType];}
    
    
    //bool                                    AnyOn(int pActionType1, int pActionType2=-1, int pActionType3=-1, int pActionType4=-1, int pActionType5=-1, int pActionType6=-1, int pActionType7=-1, int pActionType8=-1){return ((mActionTick[pActionType] > 0) || (mActionRequest[pActionType] > 0));}
    
    
    /////////////////////////////////////////////////////
    /////////////////////////////////////////////////////
    ////
    ////     Grid creation, resizing, etc etc.
    ////
    
    GameTile                                ***mTile;
    
    int                                     mGridWidth;
    int                                     mGridHeight;
    
    bool                                    OnGrid(int pGridX, int pGridY);
    
    bool                                    CanTopple(int pGridX, int pGridY);
    bool                                    CanLand(int pGridX, int pGridY);
    
    
    bool                                    SetTile(int pGridX, int pGridY, GameTile *pTile);
    bool                                    CanSetTile(int pGridX, int pGridY, GameTile *pTile);
    
    GameTile                                *GetTile(int pGridX, int pGridY);
    GameTile                                *GetTile(int pGridX, int pGridY, int pTileType);
    GameTileMatchable                       *GetTileMatchable(int pGridX, int pGridY){return (GameTileMatchable *)(GetTile(pGridX, pGridY, GAME_TILE_TYPE_MATCHABLE));}
    
    GameTileMatchable                       *GetRandomMatchableTile();
    
    GameTile                                *RemoveTile(int pGridX, int pGridY);
    void                                    DeleteTile(int pGridX, int pGridY);
    
    
    bool                                    Topple();
    
    bool                                    FillHoles();
    
    void                                    RandomizeBoard();
    void                                    RandomizeNewTiles();
    
    
    int                                     mEffectTwinkleCooldown[3];
    int                                     mEffectWiggleCooldown[3];
    int                                     mEffectTick;
    
    
    ////////////////////////////
    //
    //  Rectangular stuff
    //
    
    void                                    SizeGrid(int pGridWidth, int pGridHeight);
    void                                    ResizeGrid(int pGridWidth, int pGridHeight);
    
    void                                    PadGridTop(int pBlockCount){PadGrid(pBlockCount, 0);}
    void                                    PadGridBottom(int pBlockCount){PadGrid(0, pBlockCount);}
    void                                    PadGrid(int pBlockCountTop, int pBlockCountBottom);
    
    //
    ////////////////////////////
    
};

extern GameBoard *gBoard;

#endif








