cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "" FORCE)
set(CMAKE_GENERATOR_PLATFORM Win32)
set(CMAKE_VS_PLATFORM_NAME "Win32")
set(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE x86)
set(CMAKE_GENERATOR_TOOLSET "host=x86")
set(CMAKE_CXX_COMPILER_ARCHITECTURE_ID x86)
set(MSVC_CXX_ARCHITECTURE_ID x86)

project(GothicCoop CXX)

################################################################################
# Global configuration types
################################################################################
set(CMAKE_CONFIGURATION_TYPES
    "G1ADebug"
    "G1AMDRelease"
    "G1AMTRelease"
    "G1Debug"
    "G1MDRelease"
    "G1MTRelease"
    "G2ADebug"
    "G2AMDRelease"
    "G2AMTRelease"
    "G2Debug"
    "G2MDRelease"
    "G2MTRelease"
    "MPx2Debug"
    "MPx2MDRelease"
    "MPx2MTRelease"
    "MPx4Debug"
    "MPx4MDRelease"
    "MPx4MTRelease"
    CACHE STRING "" FORCE
)

################################################################################
# Global compiler options
################################################################################
if(MSVC)
    # remove default flags provided with CMake for MSVC
    set(CMAKE_CXX_FLAGS "")
    set(CMAKE_CXX_FLAGS_G1ADEBUG "")
    set(CMAKE_CXX_FLAGS_G1AMDRELEASE "")
    set(CMAKE_CXX_FLAGS_G1AMTRELEASE "")
    set(CMAKE_CXX_FLAGS_G1DEBUG "")
    set(CMAKE_CXX_FLAGS_G1MDRELEASE "")
    set(CMAKE_CXX_FLAGS_G1MTRELEASE "")
    set(CMAKE_CXX_FLAGS_G2ADEBUG "")
    set(CMAKE_CXX_FLAGS_G2AMDRELEASE "")
    set(CMAKE_CXX_FLAGS_G2AMTRELEASE "")
    set(CMAKE_CXX_FLAGS_G2DEBUG "")
    set(CMAKE_CXX_FLAGS_G2MDRELEASE "")
    set(CMAKE_CXX_FLAGS_G2MTRELEASE "")
    set(CMAKE_CXX_FLAGS_MPX2DEBUG "")
    set(CMAKE_CXX_FLAGS_MPX2MDRELEASE "")
    set(CMAKE_CXX_FLAGS_MPX2MTRELEASE "")
    set(CMAKE_CXX_FLAGS_MPX4DEBUG "")
    set(CMAKE_CXX_FLAGS_MPX4MDRELEASE "")
    set(CMAKE_CXX_FLAGS_MPX4MTRELEASE "")
endif()

################################################################################
# Global linker options
################################################################################
if(MSVC)
    # remove default flags provided with CMake for MSVC
    set(CMAKE_EXE_LINKER_FLAGS "")
    set(CMAKE_MODULE_LINKER_FLAGS "")
    set(CMAKE_SHARED_LINKER_FLAGS "")
    set(CMAKE_STATIC_LINKER_FLAGS "")
    set(CMAKE_EXE_LINKER_FLAGS_G1ADEBUG "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_G1ADEBUG "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_G1ADEBUG "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_G1ADEBUG "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_G1AMDRELEASE "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_G1AMDRELEASE "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_G1AMDRELEASE "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_G1AMDRELEASE "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_G1AMTRELEASE "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_G1AMTRELEASE "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_G1AMTRELEASE "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_G1AMTRELEASE "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_G1DEBUG "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_G1DEBUG "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_G1DEBUG "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_G1DEBUG "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_G1MDRELEASE "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_G1MDRELEASE "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_G1MDRELEASE "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_G1MDRELEASE "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_G1MTRELEASE "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_G1MTRELEASE "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_G1MTRELEASE "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_G1MTRELEASE "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_G2ADEBUG "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_G2ADEBUG "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_G2ADEBUG "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_G2ADEBUG "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_G2AMDRELEASE "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_G2AMDRELEASE "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_G2AMDRELEASE "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_G2AMDRELEASE "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_G2AMTRELEASE "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_G2AMTRELEASE "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_G2AMTRELEASE "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_G2AMTRELEASE "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_G2DEBUG "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_G2DEBUG "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_G2DEBUG "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_G2DEBUG "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_G2MDRELEASE "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_G2MDRELEASE "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_G2MDRELEASE "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_G2MDRELEASE "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_G2MTRELEASE "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_G2MTRELEASE "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_G2MTRELEASE "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_G2MTRELEASE "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_MPX2DEBUG "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_MPX2DEBUG "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_MPX2DEBUG "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_MPX2DEBUG "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_MPX2MDRELEASE "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_MPX2MDRELEASE "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_MPX2MDRELEASE "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_MPX2MDRELEASE "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_MPX2MTRELEASE "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_MPX2MTRELEASE "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_MPX2MTRELEASE "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_MPX2MTRELEASE "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_MPX4DEBUG "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_MPX4DEBUG "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_MPX4DEBUG "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_MPX4DEBUG "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_MPX4MDRELEASE "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_MPX4MDRELEASE "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_MPX4MDRELEASE "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_MPX4MDRELEASE "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_MPX4MTRELEASE "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_MPX4MTRELEASE "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_MPX4MTRELEASE "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_MPX4MTRELEASE "${CMAKE_STATIC_LINKER_FLAGS}")
endif()

################################################################################
# Nuget packages function stub.
################################################################################
function(use_package TARGET PACKAGE VERSION)
    message(WARNING "No implementation of use_package. Create yours. "
                    "Package \"${PACKAGE}\" with version \"${VERSION}\" "
                    "for target \"${TARGET}\" is ignored!")
endfunction()

################################################################################
# Common utils
################################################################################
include(CMake/Utils.cmake)

################################################################################
# Additional Global Settings(add specific info there)
################################################################################
include(CMake/GlobalSettingsInclude.cmake OPTIONAL)

################################################################################
# Use solution folders feature
################################################################################
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

################################################################################
# Sub-projects
################################################################################



























set(PROJECT_NAME GothicCoop)

################################################################################
# Source groups
################################################################################
set(no_group_source_files
    "resource.h"
)
source_group("" FILES ${no_group_source_files})

set(Engine_SDK
    "ZenGin/zGothicAPI.h"
)
source_group("Engine SDK" FILES ${Engine_SDK})

set(Engine_SDK__1._Gothic_I
    "ZenGin/Gothic_I_Classic/Names_G1.hpp"
)
source_group("Engine SDK\\1. Gothic I" FILES ${Engine_SDK__1._Gothic_I})

set(Engine_SDK__1._Gothic_I__Classes__oGame
    "ZenGin/Gothic_I_Classic/API/oAiHuman.h"
    "ZenGin/Gothic_I_Classic/API/oAiShoot.h"
    "ZenGin/Gothic_I_Classic/API/oAniCtrl.h"
    "ZenGin/Gothic_I_Classic/API/oBarrier.h"
    "ZenGin/Gothic_I_Classic/API/oBinkPlayer.h"
    "ZenGin/Gothic_I_Classic/API/oCollisionObjectMiscChar.h"
    "ZenGin/Gothic_I_Classic/API/oCsManager.h"
    "ZenGin/Gothic_I_Classic/API/oCsPlayer.h"
    "ZenGin/Gothic_I_Classic/API/oCsProps.h"
    "ZenGin/Gothic_I_Classic/API/oCsTrigger.h"
    "ZenGin/Gothic_I_Classic/API/oDamage.h"
    "ZenGin/Gothic_I_Classic/API/oDialog.h"
    "ZenGin/Gothic_I_Classic/API/oDoc.h"
    "ZenGin/Gothic_I_Classic/API/oDocumentManager.h"
    "ZenGin/Gothic_I_Classic/API/oFocus.h"
    "ZenGin/Gothic_I_Classic/API/oGame.h"
    "ZenGin/Gothic_I_Classic/API/oGameInfo.h"
    "ZenGin/Gothic_I_Classic/API/oGameManager.h"
    "ZenGin/Gothic_I_Classic/API/oGuilds.h"
    "ZenGin/Gothic_I_Classic/API/oHelp.h"
    "ZenGin/Gothic_I_Classic/API/oInfo.h"
    "ZenGin/Gothic_I_Classic/API/oInformationManager.h"
    "ZenGin/Gothic_I_Classic/API/oInventory.h"
    "ZenGin/Gothic_I_Classic/API/oItem.h"
    "ZenGin/Gothic_I_Classic/API/oItemReact.h"
    "ZenGin/Gothic_I_Classic/API/oLogManager.h"
    "ZenGin/Gothic_I_Classic/API/oMagFrontier.h"
    "ZenGin/Gothic_I_Classic/API/oMagic.h"
    "ZenGin/Gothic_I_Classic/API/oMenu_Help.h"
    "ZenGin/Gothic_I_Classic/API/oMenu_Main.h"
    "ZenGin/Gothic_I_Classic/API/oMenu_Status.h"
    "ZenGin/Gothic_I_Classic/API/oMenuSavegame.h"
    "ZenGin/Gothic_I_Classic/API/oMission.h"
    "ZenGin/Gothic_I_Classic/API/oMobInter.h"
    "ZenGin/Gothic_I_Classic/API/oMusicZone.h"
    "ZenGin/Gothic_I_Classic/API/oNews.h"
    "ZenGin/Gothic_I_Classic/API/oNpc.h"
    "ZenGin/Gothic_I_Classic/API/oNpcMessages.h"
    "ZenGin/Gothic_I_Classic/API/oNpcStates.h"
    "ZenGin/Gothic_I_Classic/API/oObjFactory.h"
    "ZenGin/Gothic_I_Classic/API/oParticleControl.h"
    "ZenGin/Gothic_I_Classic/API/oPlayerInfo.h"
    "ZenGin/Gothic_I_Classic/API/oPortals.h"
    "ZenGin/Gothic_I_Classic/API/oPrototypes.h"
    "ZenGin/Gothic_I_Classic/API/oRoutine.h"
    "ZenGin/Gothic_I_Classic/API/oSavegame.h"
    "ZenGin/Gothic_I_Classic/API/oSpawn.h"
    "ZenGin/Gothic_I_Classic/API/oSpell.h"
    "ZenGin/Gothic_I_Classic/API/oSvm.h"
    "ZenGin/Gothic_I_Classic/API/oSysInfo.h"
    "ZenGin/Gothic_I_Classic/API/oText.h"
    "ZenGin/Gothic_I_Classic/API/oTrigger.h"
    "ZenGin/Gothic_I_Classic/API/oViewDialogInventory.h"
    "ZenGin/Gothic_I_Classic/API/oViewDialogItem.h"
    "ZenGin/Gothic_I_Classic/API/oViewDialogItemContainer.h"
    "ZenGin/Gothic_I_Classic/API/oViewDialogStealContainer.h"
    "ZenGin/Gothic_I_Classic/API/oViewDialogTrade.h"
    "ZenGin/Gothic_I_Classic/API/oViewDocument.h"
    "ZenGin/Gothic_I_Classic/API/oViewDocumentMap.h"
    "ZenGin/Gothic_I_Classic/API/oViewProgressbar.h"
    "ZenGin/Gothic_I_Classic/API/oViewStatusbar.h"
    "ZenGin/Gothic_I_Classic/API/oVisFx.h"
    "ZenGin/Gothic_I_Classic/API/oVisFx_MultiTarget.h"
    "ZenGin/Gothic_I_Classic/API/oVob.h"
    "ZenGin/Gothic_I_Classic/API/oWaynet.h"
    "ZenGin/Gothic_I_Classic/API/oWorld.h"
)
source_group("Engine SDK\\1. Gothic I\\Classes\\oGame" FILES ${Engine_SDK__1._Gothic_I__Classes__oGame})

set(Engine_SDK__1._Gothic_I__Classes__zEngine
    "ZenGin/Gothic_I_Classic/API/z3d.h"
    "ZenGin/Gothic_I_Classic/API/zAccount.h"
    "ZenGin/Gothic_I_Classic/API/zAi.h"
    "ZenGin/Gothic_I_Classic/API/zAiCamera.h"
    "ZenGin/Gothic_I_Classic/API/zAiCamera_Core.h"
    "ZenGin/Gothic_I_Classic/API/zAiCamera_Movement.h"
    "ZenGin/Gothic_I_Classic/API/zAiPlayer.h"
    "ZenGin/Gothic_I_Classic/API/zAlgebra.h"
    "ZenGin/Gothic_I_Classic/API/zArchiver.h"
    "ZenGin/Gothic_I_Classic/API/zArchiver2.h"
    "ZenGin/Gothic_I_Classic/API/zArchiverGeneric.h"
    "ZenGin/Gothic_I_Classic/API/zBinkPlayer.h"
    "ZenGin/Gothic_I_Classic/API/zBsp.h"
    "ZenGin/Gothic_I_Classic/API/zBuffer.h"
    "ZenGin/Gothic_I_Classic/API/zBVolume.h"
    "ZenGin/Gothic_I_Classic/API/zBVolume_Util.h"
    "ZenGin/Gothic_I_Classic/API/zCCsContext.h"
    "ZenGin/Gothic_I_Classic/API/zCCsCutscene.h"
    "ZenGin/Gothic_I_Classic/API/zCCsLib.h"
    "ZenGin/Gothic_I_Classic/API/zCCsManager.h"
    "ZenGin/Gothic_I_Classic/API/zCCsPlayer.h"
    "ZenGin/Gothic_I_Classic/API/zCCsPool.h"
    "ZenGin/Gothic_I_Classic/API/zCCsProps.h"
    "ZenGin/Gothic_I_Classic/API/zCollisionDetector.h"
    "ZenGin/Gothic_I_Classic/API/zCollisionObject.h"
    "ZenGin/Gothic_I_Classic/API/zCollisionObjectMisc.h"
    "ZenGin/Gothic_I_Classic/API/zConsole.h"
    "ZenGin/Gothic_I_Classic/API/zContainer.h"
    "ZenGin/Gothic_I_Classic/API/zConvexCollision.h"
    "ZenGin/Gothic_I_Classic/API/zCsCamera.h"
    "ZenGin/Gothic_I_Classic/API/zDisk.h"
    "ZenGin/Gothic_I_Classic/API/zDisk_Vdfs.h"
    "ZenGin/Gothic_I_Classic/API/zEigen.h"
    "ZenGin/Gothic_I_Classic/API/zEngine.h"
    "ZenGin/Gothic_I_Classic/API/zError.h"
    "ZenGin/Gothic_I_Classic/API/zEventMan.h"
    "ZenGin/Gothic_I_Classic/API/zFft.h"
    "ZenGin/Gothic_I_Classic/API/zFile3d.h"
    "ZenGin/Gothic_I_Classic/API/zFileCsv.h"
    "ZenGin/Gothic_I_Classic/API/zFlash.h"
    "ZenGin/Gothic_I_Classic/API/zFonts.h"
    "ZenGin/Gothic_I_Classic/API/zGameInfo.h"
    "ZenGin/Gothic_I_Classic/API/zGlobal.h"
    "ZenGin/Gothic_I_Classic/API/zGrafix.h"
    "ZenGin/Gothic_I_Classic/API/zInput.h"
    "ZenGin/Gothic_I_Classic/API/zInput_Const.h"
    "ZenGin/Gothic_I_Classic/API/zInput_Win32.h"
    "ZenGin/Gothic_I_Classic/API/zKbSplines.h"
    "ZenGin/Gothic_I_Classic/API/zLensflare.h"
    "ZenGin/Gothic_I_Classic/API/zList.h"
    "ZenGin/Gothic_I_Classic/API/zMacro.h"
    "ZenGin/Gothic_I_Classic/API/zMaterial.h"
    "ZenGin/Gothic_I_Classic/API/zMathTools.h"
    "ZenGin/Gothic_I_Classic/API/zMemory_Base.h"
    "ZenGin/Gothic_I_Classic/API/zMemory_Generic.h"
    "ZenGin/Gothic_I_Classic/API/zMemory_Win32.h"
    "ZenGin/Gothic_I_Classic/API/zMemPool.h"
    "ZenGin/Gothic_I_Classic/API/zMenu.h"
    "ZenGin/Gothic_I_Classic/API/zMenu_Item.h"
    "ZenGin/Gothic_I_Classic/API/zModel.h"
    "ZenGin/Gothic_I_Classic/API/zMorphMesh.h"
    "ZenGin/Gothic_I_Classic/API/zMoving.h"
    "ZenGin/Gothic_I_Classic/API/zMusic.h"
    "ZenGin/Gothic_I_Classic/API/zMusic_Dm.h"
    "ZenGin/Gothic_I_Classic/API/zMusicCtrl.h"
    "ZenGin/Gothic_I_Classic/API/zNet.h"
    "ZenGin/Gothic_I_Classic/API/zNet_Win32.h"
    "ZenGin/Gothic_I_Classic/API/zNetEventMan.h"
    "ZenGin/Gothic_I_Classic/API/zNetHost.h"
    "ZenGin/Gothic_I_Classic/API/zNetManager.h"
    "ZenGin/Gothic_I_Classic/API/zNetMessage.h"
    "ZenGin/Gothic_I_Classic/API/zNetSession.h"
    "ZenGin/Gothic_I_Classic/API/zNetTypes.h"
    "ZenGin/Gothic_I_Classic/API/zNetVersion.h"
    "ZenGin/Gothic_I_Classic/API/zNetVobControl.h"
    "ZenGin/Gothic_I_Classic/API/zObject.h"
    "ZenGin/Gothic_I_Classic/API/zOption.h"
    "ZenGin/Gothic_I_Classic/API/zParser.h"
    "ZenGin/Gothic_I_Classic/API/zParser_Const.h"
    "ZenGin/Gothic_I_Classic/API/zParser_Symbol.h"
    "ZenGin/Gothic_I_Classic/API/zParticle.h"
    "ZenGin/Gothic_I_Classic/API/zPhysics.h"
    "ZenGin/Gothic_I_Classic/API/zPlayerGroup.h"
    "ZenGin/Gothic_I_Classic/API/zPlayerInfo.h"
    "ZenGin/Gothic_I_Classic/API/zPluecker.h"
    "ZenGin/Gothic_I_Classic/API/zPolyPool.h"
    "ZenGin/Gothic_I_Classic/API/zPolyStrip.h"
    "ZenGin/Gothic_I_Classic/API/zPrime.h"
    "ZenGin/Gothic_I_Classic/API/zProgMesh.h"
    "ZenGin/Gothic_I_Classic/API/zPrototypes.h"
    "ZenGin/Gothic_I_Classic/API/zQueue.h"
    "ZenGin/Gothic_I_Classic/API/zRayCache.h"
    "ZenGin/Gothic_I_Classic/API/zRenderer.h"
    "ZenGin/Gothic_I_Classic/API/zRenderLightContainer.h"
    "ZenGin/Gothic_I_Classic/API/zRenderManager.h"
    "ZenGin/Gothic_I_Classic/API/zResource.h"
    "ZenGin/Gothic_I_Classic/API/zRndD3d.h"
    "ZenGin/Gothic_I_Classic/API/zRndD3d_Init.h"
    "ZenGin/Gothic_I_Classic/API/zRndGlide.h"
    "ZenGin/Gothic_I_Classic/API/zRndGlide_Dib.h"
    "ZenGin/Gothic_I_Classic/API/zRndGlide_Draw.h"
    "ZenGin/Gothic_I_Classic/API/zScanDir.h"
    "ZenGin/Gothic_I_Classic/API/zSession.h"
    "ZenGin/Gothic_I_Classic/API/zSky.h"
    "ZenGin/Gothic_I_Classic/API/zSky_Outdoor.h"
    "ZenGin/Gothic_I_Classic/API/zSndMss.h"
    "ZenGin/Gothic_I_Classic/API/zSound.h"
    "ZenGin/Gothic_I_Classic/API/zSoundMan.h"
    "ZenGin/Gothic_I_Classic/API/zSparseArray.h"
    "ZenGin/Gothic_I_Classic/API/zSpyTypes.h"
    "ZenGin/Gothic_I_Classic/API/zString.h"
    "ZenGin/Gothic_I_Classic/API/zSysInfo.h"
    "ZenGin/Gothic_I_Classic/API/zTexConvert.h"
    "ZenGin/Gothic_I_Classic/API/zTexture.h"
    "ZenGin/Gothic_I_Classic/API/zThread.h"
    "ZenGin/Gothic_I_Classic/API/zTools.h"
    "ZenGin/Gothic_I_Classic/API/zTypes.h"
    "ZenGin/Gothic_I_Classic/API/zTypes3d.h"
    "ZenGin/Gothic_I_Classic/API/zVertexBuffer.h"
    "ZenGin/Gothic_I_Classic/API/zVertexTransform.h"
    "ZenGin/Gothic_I_Classic/API/zVideo_Win32.h"
    "ZenGin/Gothic_I_Classic/API/zVideoPlayer.h"
    "ZenGin/Gothic_I_Classic/API/zView.h"
    "ZenGin/Gothic_I_Classic/API/zViewBase.h"
    "ZenGin/Gothic_I_Classic/API/zViewDialog.h"
    "ZenGin/Gothic_I_Classic/API/zViewDialogChoice.h"
    "ZenGin/Gothic_I_Classic/API/zViewDraw.h"
    "ZenGin/Gothic_I_Classic/API/zViewFx.h"
    "ZenGin/Gothic_I_Classic/API/zViewObject.h"
    "ZenGin/Gothic_I_Classic/API/zViewPrint.h"
    "ZenGin/Gothic_I_Classic/API/zViewPrint_Font.h"
    "ZenGin/Gothic_I_Classic/API/zViewProgressbar.h"
    "ZenGin/Gothic_I_Classic/API/zViews.h"
    "ZenGin/Gothic_I_Classic/API/zViewStatusbar.h"
    "ZenGin/Gothic_I_Classic/API/zVisual.h"
    "ZenGin/Gothic_I_Classic/API/zVob.h"
    "ZenGin/Gothic_I_Classic/API/zVobIdentifier.h"
    "ZenGin/Gothic_I_Classic/API/zVobMisc.h"
    "ZenGin/Gothic_I_Classic/API/zWaynet.h"
    "ZenGin/Gothic_I_Classic/API/zWorld.h"
    "ZenGin/Gothic_I_Classic/API/zWorldInfo.h"
    "ZenGin/Gothic_I_Classic/API/zZone.h"
)
source_group("Engine SDK\\1. Gothic I\\Classes\\zEngine" FILES ${Engine_SDK__1._Gothic_I__Classes__zEngine})

set(Engine_SDK__1._Gothic_I__Overloads
    "ZenGin/Gothic_I_Classic/ClassDeclarators_G1.cpp"
    "ZenGin/Gothic_I_Classic/Globals_G1.cpp"
    "ZenGin/Gothic_I_Classic/Statics_G1.cpp"
    "ZenGin/Gothic_I_Classic/VFTable_G1.h"
)
source_group("Engine SDK\\1. Gothic I\\Overloads" FILES ${Engine_SDK__1._Gothic_I__Overloads})

set(Engine_SDK__2._Gothic_Sequel
    "ZenGin/Gothic_I_Addon/Names_G1A.hpp"
)
source_group("Engine SDK\\2. Gothic Sequel" FILES ${Engine_SDK__2._Gothic_Sequel})

set(Engine_SDK__2._Gothic_Sequel__Headers__oGame
    "ZenGin/Gothic_I_Addon/API/oAiHuman.h"
    "ZenGin/Gothic_I_Addon/API/oAiShoot.h"
    "ZenGin/Gothic_I_Addon/API/oAniCtrl.h"
    "ZenGin/Gothic_I_Addon/API/oBarrier.h"
    "ZenGin/Gothic_I_Addon/API/oBinkPlayer.h"
    "ZenGin/Gothic_I_Addon/API/oCollisionObjectMiscChar.h"
    "ZenGin/Gothic_I_Addon/API/oCsManager.h"
    "ZenGin/Gothic_I_Addon/API/oCsPlayer.h"
    "ZenGin/Gothic_I_Addon/API/oCsProps.h"
    "ZenGin/Gothic_I_Addon/API/oCsTrigger.h"
    "ZenGin/Gothic_I_Addon/API/oDamage.h"
    "ZenGin/Gothic_I_Addon/API/oDialog.h"
    "ZenGin/Gothic_I_Addon/API/oDoc.h"
    "ZenGin/Gothic_I_Addon/API/oDocumentManager.h"
    "ZenGin/Gothic_I_Addon/API/oFocus.h"
    "ZenGin/Gothic_I_Addon/API/oGame.h"
    "ZenGin/Gothic_I_Addon/API/oGameInfo.h"
    "ZenGin/Gothic_I_Addon/API/oGameManager.h"
    "ZenGin/Gothic_I_Addon/API/oGuilds.h"
    "ZenGin/Gothic_I_Addon/API/oHelp.h"
    "ZenGin/Gothic_I_Addon/API/oInfo.h"
    "ZenGin/Gothic_I_Addon/API/oInformationManager.h"
    "ZenGin/Gothic_I_Addon/API/oInventory.h"
    "ZenGin/Gothic_I_Addon/API/oItem.h"
    "ZenGin/Gothic_I_Addon/API/oItemReact.h"
    "ZenGin/Gothic_I_Addon/API/oLogManager.h"
    "ZenGin/Gothic_I_Addon/API/oMagFrontier.h"
    "ZenGin/Gothic_I_Addon/API/oMagic.h"
    "ZenGin/Gothic_I_Addon/API/oMenu_Help.h"
    "ZenGin/Gothic_I_Addon/API/oMenu_Main.h"
    "ZenGin/Gothic_I_Addon/API/oMenu_Status.h"
    "ZenGin/Gothic_I_Addon/API/oMenuSavegame.h"
    "ZenGin/Gothic_I_Addon/API/oMission.h"
    "ZenGin/Gothic_I_Addon/API/oMobInter.h"
    "ZenGin/Gothic_I_Addon/API/oMusicZone.h"
    "ZenGin/Gothic_I_Addon/API/oNews.h"
    "ZenGin/Gothic_I_Addon/API/oNpc.h"
    "ZenGin/Gothic_I_Addon/API/oNpcMessages.h"
    "ZenGin/Gothic_I_Addon/API/oNpcStates.h"
    "ZenGin/Gothic_I_Addon/API/oObjFactory.h"
    "ZenGin/Gothic_I_Addon/API/oParticleControl.h"
    "ZenGin/Gothic_I_Addon/API/oPlayerInfo.h"
    "ZenGin/Gothic_I_Addon/API/oPortals.h"
    "ZenGin/Gothic_I_Addon/API/oPrototypes.h"
    "ZenGin/Gothic_I_Addon/API/oRoutine.h"
    "ZenGin/Gothic_I_Addon/API/oSavegame.h"
    "ZenGin/Gothic_I_Addon/API/oSpawn.h"
    "ZenGin/Gothic_I_Addon/API/oSpell.h"
    "ZenGin/Gothic_I_Addon/API/oSvm.h"
    "ZenGin/Gothic_I_Addon/API/oSysInfo.h"
    "ZenGin/Gothic_I_Addon/API/oText.h"
    "ZenGin/Gothic_I_Addon/API/oTrigger.h"
    "ZenGin/Gothic_I_Addon/API/oViewDialogInventory.h"
    "ZenGin/Gothic_I_Addon/API/oViewDialogItem.h"
    "ZenGin/Gothic_I_Addon/API/oViewDialogItemContainer.h"
    "ZenGin/Gothic_I_Addon/API/oViewDialogStealContainer.h"
    "ZenGin/Gothic_I_Addon/API/oViewDialogTrade.h"
    "ZenGin/Gothic_I_Addon/API/oViewDocument.h"
    "ZenGin/Gothic_I_Addon/API/oViewDocumentMap.h"
    "ZenGin/Gothic_I_Addon/API/oViewProgressbar.h"
    "ZenGin/Gothic_I_Addon/API/oViewStatusbar.h"
    "ZenGin/Gothic_I_Addon/API/oVisFx.h"
    "ZenGin/Gothic_I_Addon/API/oVisFx_MultiTarget.h"
    "ZenGin/Gothic_I_Addon/API/oVob.h"
    "ZenGin/Gothic_I_Addon/API/oWaynet.h"
    "ZenGin/Gothic_I_Addon/API/oWorld.h"
)
source_group("Engine SDK\\2. Gothic Sequel\\Headers\\oGame" FILES ${Engine_SDK__2._Gothic_Sequel__Headers__oGame})

set(Engine_SDK__2._Gothic_Sequel__Headers__zEngine
    "ZenGin/Gothic_I_Addon/API/z3d.h"
    "ZenGin/Gothic_I_Addon/API/zAccount.h"
    "ZenGin/Gothic_I_Addon/API/zAi.h"
    "ZenGin/Gothic_I_Addon/API/zAiCamera.h"
    "ZenGin/Gothic_I_Addon/API/zAiCamera_Core.h"
    "ZenGin/Gothic_I_Addon/API/zAiCamera_Movement.h"
    "ZenGin/Gothic_I_Addon/API/zAiPlayer.h"
    "ZenGin/Gothic_I_Addon/API/zAlgebra.h"
    "ZenGin/Gothic_I_Addon/API/zArchiver.h"
    "ZenGin/Gothic_I_Addon/API/zArchiver2.h"
    "ZenGin/Gothic_I_Addon/API/zArchiverGeneric.h"
    "ZenGin/Gothic_I_Addon/API/zBinkPlayer.h"
    "ZenGin/Gothic_I_Addon/API/zBsp.h"
    "ZenGin/Gothic_I_Addon/API/zBuffer.h"
    "ZenGin/Gothic_I_Addon/API/zBVolume.h"
    "ZenGin/Gothic_I_Addon/API/zBVolume_Util.h"
    "ZenGin/Gothic_I_Addon/API/zCCsContext.h"
    "ZenGin/Gothic_I_Addon/API/zCCsCutscene.h"
    "ZenGin/Gothic_I_Addon/API/zCCsLib.h"
    "ZenGin/Gothic_I_Addon/API/zCCsManager.h"
    "ZenGin/Gothic_I_Addon/API/zCCsPlayer.h"
    "ZenGin/Gothic_I_Addon/API/zCCsPool.h"
    "ZenGin/Gothic_I_Addon/API/zCCsProps.h"
    "ZenGin/Gothic_I_Addon/API/zCollisionDetector.h"
    "ZenGin/Gothic_I_Addon/API/zCollisionObject.h"
    "ZenGin/Gothic_I_Addon/API/zCollisionObjectMisc.h"
    "ZenGin/Gothic_I_Addon/API/zConsole.h"
    "ZenGin/Gothic_I_Addon/API/zContainer.h"
    "ZenGin/Gothic_I_Addon/API/zConvexCollision.h"
    "ZenGin/Gothic_I_Addon/API/zCsCamera.h"
    "ZenGin/Gothic_I_Addon/API/zDisk.h"
    "ZenGin/Gothic_I_Addon/API/zDisk_Vdfs.h"
    "ZenGin/Gothic_I_Addon/API/zEigen.h"
    "ZenGin/Gothic_I_Addon/API/zEngine.h"
    "ZenGin/Gothic_I_Addon/API/zError.h"
    "ZenGin/Gothic_I_Addon/API/zEventMan.h"
    "ZenGin/Gothic_I_Addon/API/zFile3d.h"
    "ZenGin/Gothic_I_Addon/API/zFileCsv.h"
    "ZenGin/Gothic_I_Addon/API/zFlash.h"
    "ZenGin/Gothic_I_Addon/API/zFonts.h"
    "ZenGin/Gothic_I_Addon/API/zGameInfo.h"
    "ZenGin/Gothic_I_Addon/API/zGlobal.h"
    "ZenGin/Gothic_I_Addon/API/zGrafix.h"
    "ZenGin/Gothic_I_Addon/API/zInput.h"
    "ZenGin/Gothic_I_Addon/API/zInput_Const.h"
    "ZenGin/Gothic_I_Addon/API/zInput_Win32.h"
    "ZenGin/Gothic_I_Addon/API/zKbSplines.h"
    "ZenGin/Gothic_I_Addon/API/zLensflare.h"
    "ZenGin/Gothic_I_Addon/API/zList.h"
    "ZenGin/Gothic_I_Addon/API/zMacro.h"
    "ZenGin/Gothic_I_Addon/API/zMaterial.h"
    "ZenGin/Gothic_I_Addon/API/zMathTools.h"
    "ZenGin/Gothic_I_Addon/API/zMemory_Base.h"
    "ZenGin/Gothic_I_Addon/API/zMemory_Generic.h"
    "ZenGin/Gothic_I_Addon/API/zMemory_Win32.h"
    "ZenGin/Gothic_I_Addon/API/zMemPool.h"
    "ZenGin/Gothic_I_Addon/API/zMenu.h"
    "ZenGin/Gothic_I_Addon/API/zMenu_Item.h"
    "ZenGin/Gothic_I_Addon/API/zModel.h"
    "ZenGin/Gothic_I_Addon/API/zMorphMesh.h"
    "ZenGin/Gothic_I_Addon/API/zMoving.h"
    "ZenGin/Gothic_I_Addon/API/zMusic.h"
    "ZenGin/Gothic_I_Addon/API/zMusic_Dm.h"
    "ZenGin/Gothic_I_Addon/API/zMusicCtrl.h"
    "ZenGin/Gothic_I_Addon/API/zNet.h"
    "ZenGin/Gothic_I_Addon/API/zNet_Win32.h"
    "ZenGin/Gothic_I_Addon/API/zNetEventMan.h"
    "ZenGin/Gothic_I_Addon/API/zNetHost.h"
    "ZenGin/Gothic_I_Addon/API/zNetManager.h"
    "ZenGin/Gothic_I_Addon/API/zNetMessage.h"
    "ZenGin/Gothic_I_Addon/API/zNetSession.h"
    "ZenGin/Gothic_I_Addon/API/zNetTypes.h"
    "ZenGin/Gothic_I_Addon/API/zNetVersion.h"
    "ZenGin/Gothic_I_Addon/API/zNetVobControl.h"
    "ZenGin/Gothic_I_Addon/API/zObject.h"
    "ZenGin/Gothic_I_Addon/API/zOption.h"
    "ZenGin/Gothic_I_Addon/API/zParser.h"
    "ZenGin/Gothic_I_Addon/API/zParser_Const.h"
    "ZenGin/Gothic_I_Addon/API/zParser_Symbol.h"
    "ZenGin/Gothic_I_Addon/API/zParticle.h"
    "ZenGin/Gothic_I_Addon/API/zPhysics.h"
    "ZenGin/Gothic_I_Addon/API/zPlayerGroup.h"
    "ZenGin/Gothic_I_Addon/API/zPlayerInfo.h"
    "ZenGin/Gothic_I_Addon/API/zPluecker.h"
    "ZenGin/Gothic_I_Addon/API/zPolyPool.h"
    "ZenGin/Gothic_I_Addon/API/zPolyStrip.h"
    "ZenGin/Gothic_I_Addon/API/zPrime.h"
    "ZenGin/Gothic_I_Addon/API/zProgMesh.h"
    "ZenGin/Gothic_I_Addon/API/zPrototypes.h"
    "ZenGin/Gothic_I_Addon/API/zQueue.h"
    "ZenGin/Gothic_I_Addon/API/zRayCache.h"
    "ZenGin/Gothic_I_Addon/API/zRenderer.h"
    "ZenGin/Gothic_I_Addon/API/zRenderLightContainer.h"
    "ZenGin/Gothic_I_Addon/API/zRenderManager.h"
    "ZenGin/Gothic_I_Addon/API/zResource.h"
    "ZenGin/Gothic_I_Addon/API/zRndD3d.h"
    "ZenGin/Gothic_I_Addon/API/zRndD3d_Init.h"
    "ZenGin/Gothic_I_Addon/API/zScanDir.h"
    "ZenGin/Gothic_I_Addon/API/zSession.h"
    "ZenGin/Gothic_I_Addon/API/zSky.h"
    "ZenGin/Gothic_I_Addon/API/zSky_Outdoor.h"
    "ZenGin/Gothic_I_Addon/API/zSndMss.h"
    "ZenGin/Gothic_I_Addon/API/zSound.h"
    "ZenGin/Gothic_I_Addon/API/zSoundMan.h"
    "ZenGin/Gothic_I_Addon/API/zSparseArray.h"
    "ZenGin/Gothic_I_Addon/API/zSpyTypes.h"
    "ZenGin/Gothic_I_Addon/API/zString.h"
    "ZenGin/Gothic_I_Addon/API/zSysInfo.h"
    "ZenGin/Gothic_I_Addon/API/zTexConvert.h"
    "ZenGin/Gothic_I_Addon/API/zTexture.h"
    "ZenGin/Gothic_I_Addon/API/zThread.h"
    "ZenGin/Gothic_I_Addon/API/zTools.h"
    "ZenGin/Gothic_I_Addon/API/zTypes.h"
    "ZenGin/Gothic_I_Addon/API/zTypes3d.h"
    "ZenGin/Gothic_I_Addon/API/zVertexBuffer.h"
    "ZenGin/Gothic_I_Addon/API/zVertexTransform.h"
    "ZenGin/Gothic_I_Addon/API/zVideo_Win32.h"
    "ZenGin/Gothic_I_Addon/API/zVideoPlayer.h"
    "ZenGin/Gothic_I_Addon/API/zView.h"
    "ZenGin/Gothic_I_Addon/API/zViewBase.h"
    "ZenGin/Gothic_I_Addon/API/zViewDialog.h"
    "ZenGin/Gothic_I_Addon/API/zViewDialogChoice.h"
    "ZenGin/Gothic_I_Addon/API/zViewDraw.h"
    "ZenGin/Gothic_I_Addon/API/zViewFx.h"
    "ZenGin/Gothic_I_Addon/API/zViewObject.h"
    "ZenGin/Gothic_I_Addon/API/zViewPrint.h"
    "ZenGin/Gothic_I_Addon/API/zViewPrint_Font.h"
    "ZenGin/Gothic_I_Addon/API/zViewProgressbar.h"
    "ZenGin/Gothic_I_Addon/API/zViews.h"
    "ZenGin/Gothic_I_Addon/API/zViewStatusbar.h"
    "ZenGin/Gothic_I_Addon/API/zVisual.h"
    "ZenGin/Gothic_I_Addon/API/zVob.h"
    "ZenGin/Gothic_I_Addon/API/zVobIdentifier.h"
    "ZenGin/Gothic_I_Addon/API/zVobMisc.h"
    "ZenGin/Gothic_I_Addon/API/zWaynet.h"
    "ZenGin/Gothic_I_Addon/API/zWorld.h"
    "ZenGin/Gothic_I_Addon/API/zWorldInfo.h"
    "ZenGin/Gothic_I_Addon/API/zZone.h"
)
source_group("Engine SDK\\2. Gothic Sequel\\Headers\\zEngine" FILES ${Engine_SDK__2._Gothic_Sequel__Headers__zEngine})

set(Engine_SDK__2._Gothic_Sequel__Overloads
    "ZenGin/Gothic_I_Addon/ClassDeclarators_G1A.cpp"
    "ZenGin/Gothic_I_Addon/Globals_G1A.cpp"
    "ZenGin/Gothic_I_Addon/Statics_G1A.cpp"
    "ZenGin/Gothic_I_Addon/VFTable_G1A.h"
)
source_group("Engine SDK\\2. Gothic Sequel\\Overloads" FILES ${Engine_SDK__2._Gothic_Sequel__Overloads})

set(Engine_SDK__3._Gothic_II
    "ZenGin/Gothic_II_Classic/Names_G2.hpp"
)
source_group("Engine SDK\\3. Gothic II" FILES ${Engine_SDK__3._Gothic_II})

set(Engine_SDK__3._Gothic_II__Headers__oGame
    "ZenGin/Gothic_II_Classic/API/oAiHuman.h"
    "ZenGin/Gothic_II_Classic/API/oAiShoot.h"
    "ZenGin/Gothic_II_Classic/API/oAniCtrl.h"
    "ZenGin/Gothic_II_Classic/API/oBarrier.h"
    "ZenGin/Gothic_II_Classic/API/oBinkPlayer.h"
    "ZenGin/Gothic_II_Classic/API/oCollisionObjectMiscChar.h"
    "ZenGin/Gothic_II_Classic/API/oCsManager.h"
    "ZenGin/Gothic_II_Classic/API/oCsPlayer.h"
    "ZenGin/Gothic_II_Classic/API/oCsProps.h"
    "ZenGin/Gothic_II_Classic/API/oCsTrigger.h"
    "ZenGin/Gothic_II_Classic/API/oDamage.h"
    "ZenGin/Gothic_II_Classic/API/oDialog.h"
    "ZenGin/Gothic_II_Classic/API/oDoc.h"
    "ZenGin/Gothic_II_Classic/API/oDocumentManager.h"
    "ZenGin/Gothic_II_Classic/API/oFocus.h"
    "ZenGin/Gothic_II_Classic/API/oGame.h"
    "ZenGin/Gothic_II_Classic/API/oGameInfo.h"
    "ZenGin/Gothic_II_Classic/API/oGameManager.h"
    "ZenGin/Gothic_II_Classic/API/oGuilds.h"
    "ZenGin/Gothic_II_Classic/API/oHelp.h"
    "ZenGin/Gothic_II_Classic/API/oInfo.h"
    "ZenGin/Gothic_II_Classic/API/oInformationManager.h"
    "ZenGin/Gothic_II_Classic/API/oInventory.h"
    "ZenGin/Gothic_II_Classic/API/oItem.h"
    "ZenGin/Gothic_II_Classic/API/oItemReact.h"
    "ZenGin/Gothic_II_Classic/API/oLogManager.h"
    "ZenGin/Gothic_II_Classic/API/oMagFrontier.h"
    "ZenGin/Gothic_II_Classic/API/oMagic.h"
    "ZenGin/Gothic_II_Classic/API/oMenu_Help.h"
    "ZenGin/Gothic_II_Classic/API/oMenu_Main.h"
    "ZenGin/Gothic_II_Classic/API/oMenu_Status.h"
    "ZenGin/Gothic_II_Classic/API/oMenuSavegame.h"
    "ZenGin/Gothic_II_Classic/API/oMission.h"
    "ZenGin/Gothic_II_Classic/API/oMobInter.h"
    "ZenGin/Gothic_II_Classic/API/oMusicZone.h"
    "ZenGin/Gothic_II_Classic/API/oNews.h"
    "ZenGin/Gothic_II_Classic/API/oNpc.h"
    "ZenGin/Gothic_II_Classic/API/oNpcMessages.h"
    "ZenGin/Gothic_II_Classic/API/oNpcStates.h"
    "ZenGin/Gothic_II_Classic/API/oObjFactory.h"
    "ZenGin/Gothic_II_Classic/API/oParticleControl.h"
    "ZenGin/Gothic_II_Classic/API/oPlayerInfo.h"
    "ZenGin/Gothic_II_Classic/API/oPortals.h"
    "ZenGin/Gothic_II_Classic/API/oPrototypes.h"
    "ZenGin/Gothic_II_Classic/API/oRoutine.h"
    "ZenGin/Gothic_II_Classic/API/oSavegame.h"
    "ZenGin/Gothic_II_Classic/API/oSpawn.h"
    "ZenGin/Gothic_II_Classic/API/oSpell.h"
    "ZenGin/Gothic_II_Classic/API/oSvm.h"
    "ZenGin/Gothic_II_Classic/API/oSysInfo.h"
    "ZenGin/Gothic_II_Classic/API/oText.h"
    "ZenGin/Gothic_II_Classic/API/oTrigger.h"
    "ZenGin/Gothic_II_Classic/API/oViewDialogInventory.h"
    "ZenGin/Gothic_II_Classic/API/oViewDialogItem.h"
    "ZenGin/Gothic_II_Classic/API/oViewDialogItemContainer.h"
    "ZenGin/Gothic_II_Classic/API/oViewDialogStealContainer.h"
    "ZenGin/Gothic_II_Classic/API/oViewDialogTrade.h"
    "ZenGin/Gothic_II_Classic/API/oViewDocument.h"
    "ZenGin/Gothic_II_Classic/API/oViewDocumentMap.h"
    "ZenGin/Gothic_II_Classic/API/oViewProgressbar.h"
    "ZenGin/Gothic_II_Classic/API/oViewStatusbar.h"
    "ZenGin/Gothic_II_Classic/API/oVisFx.h"
    "ZenGin/Gothic_II_Classic/API/oVisFx_MultiTarget.h"
    "ZenGin/Gothic_II_Classic/API/oVob.h"
    "ZenGin/Gothic_II_Classic/API/oWaynet.h"
    "ZenGin/Gothic_II_Classic/API/oWorld.h"
)
source_group("Engine SDK\\3. Gothic II\\Headers\\oGame" FILES ${Engine_SDK__3._Gothic_II__Headers__oGame})

set(Engine_SDK__3._Gothic_II__Headers__zEngine
    "ZenGin/Gothic_II_Classic/API/z3d.h"
    "ZenGin/Gothic_II_Classic/API/zAccount.h"
    "ZenGin/Gothic_II_Classic/API/zAi.h"
    "ZenGin/Gothic_II_Classic/API/zAiCamera.h"
    "ZenGin/Gothic_II_Classic/API/zAiCamera_Core.h"
    "ZenGin/Gothic_II_Classic/API/zAiCamera_Movement.h"
    "ZenGin/Gothic_II_Classic/API/zAiPlayer.h"
    "ZenGin/Gothic_II_Classic/API/zAlgebra.h"
    "ZenGin/Gothic_II_Classic/API/zArchiver.h"
    "ZenGin/Gothic_II_Classic/API/zArchiver2.h"
    "ZenGin/Gothic_II_Classic/API/zArchiverGeneric.h"
    "ZenGin/Gothic_II_Classic/API/zBinkPlayer.h"
    "ZenGin/Gothic_II_Classic/API/zBsp.h"
    "ZenGin/Gothic_II_Classic/API/zBuffer.h"
    "ZenGin/Gothic_II_Classic/API/zBVolume.h"
    "ZenGin/Gothic_II_Classic/API/zBVolume_Util.h"
    "ZenGin/Gothic_II_Classic/API/zCCsContext.h"
    "ZenGin/Gothic_II_Classic/API/zCCsCutscene.h"
    "ZenGin/Gothic_II_Classic/API/zCCsLib.h"
    "ZenGin/Gothic_II_Classic/API/zCCsManager.h"
    "ZenGin/Gothic_II_Classic/API/zCCsPlayer.h"
    "ZenGin/Gothic_II_Classic/API/zCCsPool.h"
    "ZenGin/Gothic_II_Classic/API/zCCsProps.h"
    "ZenGin/Gothic_II_Classic/API/zCollisionDetector.h"
    "ZenGin/Gothic_II_Classic/API/zCollisionObject.h"
    "ZenGin/Gothic_II_Classic/API/zCollisionObjectMisc.h"
    "ZenGin/Gothic_II_Classic/API/zConsole.h"
    "ZenGin/Gothic_II_Classic/API/zContainer.h"
    "ZenGin/Gothic_II_Classic/API/zConvexCollision.h"
    "ZenGin/Gothic_II_Classic/API/zCsCamera.h"
    "ZenGin/Gothic_II_Classic/API/zDisk.h"
    "ZenGin/Gothic_II_Classic/API/zDisk_Vdfs.h"
    "ZenGin/Gothic_II_Classic/API/zEigen.h"
    "ZenGin/Gothic_II_Classic/API/zEngine.h"
    "ZenGin/Gothic_II_Classic/API/zError.h"
    "ZenGin/Gothic_II_Classic/API/zEventMan.h"
    "ZenGin/Gothic_II_Classic/API/zFft.h"
    "ZenGin/Gothic_II_Classic/API/zFile3d.h"
    "ZenGin/Gothic_II_Classic/API/zFileCsv.h"
    "ZenGin/Gothic_II_Classic/API/zFlash.h"
    "ZenGin/Gothic_II_Classic/API/zFonts.h"
    "ZenGin/Gothic_II_Classic/API/zGameInfo.h"
    "ZenGin/Gothic_II_Classic/API/zGlobal.h"
    "ZenGin/Gothic_II_Classic/API/zGrafix.h"
    "ZenGin/Gothic_II_Classic/API/zInput.h"
    "ZenGin/Gothic_II_Classic/API/zInput_Const.h"
    "ZenGin/Gothic_II_Classic/API/zInput_Win32.h"
    "ZenGin/Gothic_II_Classic/API/zKbSplines.h"
    "ZenGin/Gothic_II_Classic/API/zLensflare.h"
    "ZenGin/Gothic_II_Classic/API/zList.h"
    "ZenGin/Gothic_II_Classic/API/zLocal.h"
    "ZenGin/Gothic_II_Classic/API/zMacro.h"
    "ZenGin/Gothic_II_Classic/API/zMaterial.h"
    "ZenGin/Gothic_II_Classic/API/zMathTools.h"
    "ZenGin/Gothic_II_Classic/API/zMemory_Base.h"
    "ZenGin/Gothic_II_Classic/API/zMemory_Generic.h"
    "ZenGin/Gothic_II_Classic/API/zMemory_Win32.h"
    "ZenGin/Gothic_II_Classic/API/zMemPool.h"
    "ZenGin/Gothic_II_Classic/API/zMenu.h"
    "ZenGin/Gothic_II_Classic/API/zMenu_Item.h"
    "ZenGin/Gothic_II_Classic/API/zModel.h"
    "ZenGin/Gothic_II_Classic/API/zMorphMesh.h"
    "ZenGin/Gothic_II_Classic/API/zMoving.h"
    "ZenGin/Gothic_II_Classic/API/zMusic.h"
    "ZenGin/Gothic_II_Classic/API/zMusic_Dm.h"
    "ZenGin/Gothic_II_Classic/API/zMusicCtrl.h"
    "ZenGin/Gothic_II_Classic/API/zNet.h"
    "ZenGin/Gothic_II_Classic/API/zNet_Win32.h"
    "ZenGin/Gothic_II_Classic/API/zNetEventMan.h"
    "ZenGin/Gothic_II_Classic/API/zNetHost.h"
    "ZenGin/Gothic_II_Classic/API/zNetManager.h"
    "ZenGin/Gothic_II_Classic/API/zNetMessage.h"
    "ZenGin/Gothic_II_Classic/API/zNetSession.h"
    "ZenGin/Gothic_II_Classic/API/zNetTypes.h"
    "ZenGin/Gothic_II_Classic/API/zNetVersion.h"
    "ZenGin/Gothic_II_Classic/API/zNetVobControl.h"
    "ZenGin/Gothic_II_Classic/API/zObject.h"
    "ZenGin/Gothic_II_Classic/API/zOption.h"
    "ZenGin/Gothic_II_Classic/API/zParser.h"
    "ZenGin/Gothic_II_Classic/API/zParser_Const.h"
    "ZenGin/Gothic_II_Classic/API/zParser_Symbol.h"
    "ZenGin/Gothic_II_Classic/API/zParticle.h"
    "ZenGin/Gothic_II_Classic/API/zPhysics.h"
    "ZenGin/Gothic_II_Classic/API/zPlayerGroup.h"
    "ZenGin/Gothic_II_Classic/API/zPlayerInfo.h"
    "ZenGin/Gothic_II_Classic/API/zPluecker.h"
    "ZenGin/Gothic_II_Classic/API/zPolyPool.h"
    "ZenGin/Gothic_II_Classic/API/zPolyStrip.h"
    "ZenGin/Gothic_II_Classic/API/zPrime.h"
    "ZenGin/Gothic_II_Classic/API/zProgMesh.h"
    "ZenGin/Gothic_II_Classic/API/zPrototypes.h"
    "ZenGin/Gothic_II_Classic/API/zQueue.h"
    "ZenGin/Gothic_II_Classic/API/zRayCache.h"
    "ZenGin/Gothic_II_Classic/API/zRenderer.h"
    "ZenGin/Gothic_II_Classic/API/zRenderLightContainer.h"
    "ZenGin/Gothic_II_Classic/API/zRenderManager.h"
    "ZenGin/Gothic_II_Classic/API/zResource.h"
    "ZenGin/Gothic_II_Classic/API/zRndD3d.h"
    "ZenGin/Gothic_II_Classic/API/zRndD3d_Init.h"
    "ZenGin/Gothic_II_Classic/API/zScanDir.h"
    "ZenGin/Gothic_II_Classic/API/zSession.h"
    "ZenGin/Gothic_II_Classic/API/zSky.h"
    "ZenGin/Gothic_II_Classic/API/zSky_Outdoor.h"
    "ZenGin/Gothic_II_Classic/API/zSndMss.h"
    "ZenGin/Gothic_II_Classic/API/zSound.h"
    "ZenGin/Gothic_II_Classic/API/zSoundMan.h"
    "ZenGin/Gothic_II_Classic/API/zSparseArray.h"
    "ZenGin/Gothic_II_Classic/API/zSpyTypes.h"
    "ZenGin/Gothic_II_Classic/API/zString.h"
    "ZenGin/Gothic_II_Classic/API/zSysInfo.h"
    "ZenGin/Gothic_II_Classic/API/zTexConvert.h"
    "ZenGin/Gothic_II_Classic/API/zTexture.h"
    "ZenGin/Gothic_II_Classic/API/zThread.h"
    "ZenGin/Gothic_II_Classic/API/zTools.h"
    "ZenGin/Gothic_II_Classic/API/zTypes.h"
    "ZenGin/Gothic_II_Classic/API/zTypes3d.h"
    "ZenGin/Gothic_II_Classic/API/zVertexBuffer.h"
    "ZenGin/Gothic_II_Classic/API/zVertexTransform.h"
    "ZenGin/Gothic_II_Classic/API/zVideo_Win32.h"
    "ZenGin/Gothic_II_Classic/API/zVideoPlayer.h"
    "ZenGin/Gothic_II_Classic/API/zView.h"
    "ZenGin/Gothic_II_Classic/API/zViewBase.h"
    "ZenGin/Gothic_II_Classic/API/zViewDialog.h"
    "ZenGin/Gothic_II_Classic/API/zViewDialogChoice.h"
    "ZenGin/Gothic_II_Classic/API/zViewDraw.h"
    "ZenGin/Gothic_II_Classic/API/zViewFx.h"
    "ZenGin/Gothic_II_Classic/API/zViewObject.h"
    "ZenGin/Gothic_II_Classic/API/zViewPrint.h"
    "ZenGin/Gothic_II_Classic/API/zViewPrint_Font.h"
    "ZenGin/Gothic_II_Classic/API/zViewProgressbar.h"
    "ZenGin/Gothic_II_Classic/API/zViews.h"
    "ZenGin/Gothic_II_Classic/API/zViewStatusbar.h"
    "ZenGin/Gothic_II_Classic/API/zVisual.h"
    "ZenGin/Gothic_II_Classic/API/zVob.h"
    "ZenGin/Gothic_II_Classic/API/zVobIdentifier.h"
    "ZenGin/Gothic_II_Classic/API/zVobMisc.h"
    "ZenGin/Gothic_II_Classic/API/zWaynet.h"
    "ZenGin/Gothic_II_Classic/API/zWorld.h"
    "ZenGin/Gothic_II_Classic/API/zWorldInfo.h"
    "ZenGin/Gothic_II_Classic/API/zZone.h"
)
source_group("Engine SDK\\3. Gothic II\\Headers\\zEngine" FILES ${Engine_SDK__3._Gothic_II__Headers__zEngine})

set(Engine_SDK__3._Gothic_II__Overloads
    "ZenGin/Gothic_II_Classic/ClassDeclarators_G2.cpp"
    "ZenGin/Gothic_II_Classic/Globals_G2.cpp"
    "ZenGin/Gothic_II_Classic/Statics_G2.cpp"
    "ZenGin/Gothic_II_Classic/VFTable_G2.h"
)
source_group("Engine SDK\\3. Gothic II\\Overloads" FILES ${Engine_SDK__3._Gothic_II__Overloads})

set(Engine_SDK__4._Gothic_II_NoTR
    "ZenGin/Gothic_II_Addon/Names_G2A.hpp"
)
source_group("Engine SDK\\4. Gothic II NoTR" FILES ${Engine_SDK__4._Gothic_II_NoTR})

set(Engine_SDK__4._Gothic_II_NoTR__Headers__oGame
    "ZenGin/Gothic_II_Addon/API/oAiHuman.h"
    "ZenGin/Gothic_II_Addon/API/oAiShoot.h"
    "ZenGin/Gothic_II_Addon/API/oAniCtrl.h"
    "ZenGin/Gothic_II_Addon/API/oBarrier.h"
    "ZenGin/Gothic_II_Addon/API/oBinkPlayer.h"
    "ZenGin/Gothic_II_Addon/API/oCollisionObjectMiscChar.h"
    "ZenGin/Gothic_II_Addon/API/oCsManager.h"
    "ZenGin/Gothic_II_Addon/API/oCsPlayer.h"
    "ZenGin/Gothic_II_Addon/API/oCsProps.h"
    "ZenGin/Gothic_II_Addon/API/oCsTrigger.h"
    "ZenGin/Gothic_II_Addon/API/oDamage.h"
    "ZenGin/Gothic_II_Addon/API/oDialog.h"
    "ZenGin/Gothic_II_Addon/API/oDoc.h"
    "ZenGin/Gothic_II_Addon/API/oDocumentManager.h"
    "ZenGin/Gothic_II_Addon/API/oFocus.h"
    "ZenGin/Gothic_II_Addon/API/oGame.h"
    "ZenGin/Gothic_II_Addon/API/oGameInfo.h"
    "ZenGin/Gothic_II_Addon/API/oGameManager.h"
    "ZenGin/Gothic_II_Addon/API/oGuilds.h"
    "ZenGin/Gothic_II_Addon/API/oHelp.h"
    "ZenGin/Gothic_II_Addon/API/oInfo.h"
    "ZenGin/Gothic_II_Addon/API/oInformationManager.h"
    "ZenGin/Gothic_II_Addon/API/oInventory.h"
    "ZenGin/Gothic_II_Addon/API/oItem.h"
    "ZenGin/Gothic_II_Addon/API/oItemReact.h"
    "ZenGin/Gothic_II_Addon/API/oLogManager.h"
    "ZenGin/Gothic_II_Addon/API/oMagFrontier.h"
    "ZenGin/Gothic_II_Addon/API/oMagic.h"
    "ZenGin/Gothic_II_Addon/API/oMenu_Help.h"
    "ZenGin/Gothic_II_Addon/API/oMenu_Main.h"
    "ZenGin/Gothic_II_Addon/API/oMenu_Status.h"
    "ZenGin/Gothic_II_Addon/API/oMenuSavegame.h"
    "ZenGin/Gothic_II_Addon/API/oMission.h"
    "ZenGin/Gothic_II_Addon/API/oMobInter.h"
    "ZenGin/Gothic_II_Addon/API/oMusicZone.h"
    "ZenGin/Gothic_II_Addon/API/oNews.h"
    "ZenGin/Gothic_II_Addon/API/oNpc.h"
    "ZenGin/Gothic_II_Addon/API/oNpcMessages.h"
    "ZenGin/Gothic_II_Addon/API/oNpcStates.h"
    "ZenGin/Gothic_II_Addon/API/oObjFactory.h"
    "ZenGin/Gothic_II_Addon/API/oParticleControl.h"
    "ZenGin/Gothic_II_Addon/API/oPlayerInfo.h"
    "ZenGin/Gothic_II_Addon/API/oPortals.h"
    "ZenGin/Gothic_II_Addon/API/oPrototypes.h"
    "ZenGin/Gothic_II_Addon/API/oRoutine.h"
    "ZenGin/Gothic_II_Addon/API/oSavegame.h"
    "ZenGin/Gothic_II_Addon/API/oSpawn.h"
    "ZenGin/Gothic_II_Addon/API/oSpell.h"
    "ZenGin/Gothic_II_Addon/API/oSvm.h"
    "ZenGin/Gothic_II_Addon/API/oSysInfo.h"
    "ZenGin/Gothic_II_Addon/API/oText.h"
    "ZenGin/Gothic_II_Addon/API/oTrigger.h"
    "ZenGin/Gothic_II_Addon/API/oViewDialogInventory.h"
    "ZenGin/Gothic_II_Addon/API/oViewDialogItem.h"
    "ZenGin/Gothic_II_Addon/API/oViewDialogItemContainer.h"
    "ZenGin/Gothic_II_Addon/API/oViewDialogStealContainer.h"
    "ZenGin/Gothic_II_Addon/API/oViewDialogTrade.h"
    "ZenGin/Gothic_II_Addon/API/oViewDocument.h"
    "ZenGin/Gothic_II_Addon/API/oViewDocumentMap.h"
    "ZenGin/Gothic_II_Addon/API/oViewProgressbar.h"
    "ZenGin/Gothic_II_Addon/API/oViewStatusbar.h"
    "ZenGin/Gothic_II_Addon/API/oVisFx.h"
    "ZenGin/Gothic_II_Addon/API/oVisFx_MultiTarget.h"
    "ZenGin/Gothic_II_Addon/API/oVob.h"
    "ZenGin/Gothic_II_Addon/API/oWaynet.h"
    "ZenGin/Gothic_II_Addon/API/oWorld.h"
)
source_group("Engine SDK\\4. Gothic II NoTR\\Headers\\oGame" FILES ${Engine_SDK__4._Gothic_II_NoTR__Headers__oGame})

set(Engine_SDK__4._Gothic_II_NoTR__Headers__zEngine
    "ZenGin/Gothic_II_Addon/API/z3d.h"
    "ZenGin/Gothic_II_Addon/API/zAccount.h"
    "ZenGin/Gothic_II_Addon/API/zAi.h"
    "ZenGin/Gothic_II_Addon/API/zAiCamera.h"
    "ZenGin/Gothic_II_Addon/API/zAiCamera_Core.h"
    "ZenGin/Gothic_II_Addon/API/zAiCamera_Movement.h"
    "ZenGin/Gothic_II_Addon/API/zAIPlayer.h"
    "ZenGin/Gothic_II_Addon/API/zAlgebra.h"
    "ZenGin/Gothic_II_Addon/API/zArchiver.h"
    "ZenGin/Gothic_II_Addon/API/zArchiver2.h"
    "ZenGin/Gothic_II_Addon/API/zArchiverGeneric.h"
    "ZenGin/Gothic_II_Addon/API/zBinkPlayer.h"
    "ZenGin/Gothic_II_Addon/API/zBsp.h"
    "ZenGin/Gothic_II_Addon/API/zBuffer.h"
    "ZenGin/Gothic_II_Addon/API/zBVolume.h"
    "ZenGin/Gothic_II_Addon/API/zBVolume_Util.h"
    "ZenGin/Gothic_II_Addon/API/zCCsContext.h"
    "ZenGin/Gothic_II_Addon/API/zCCsCutscene.h"
    "ZenGin/Gothic_II_Addon/API/zCCsLib.h"
    "ZenGin/Gothic_II_Addon/API/zCCsManager.h"
    "ZenGin/Gothic_II_Addon/API/zCCsPlayer.h"
    "ZenGin/Gothic_II_Addon/API/zCCsPool.h"
    "ZenGin/Gothic_II_Addon/API/zCCsProps.h"
    "ZenGin/Gothic_II_Addon/API/zCollisionDetector.h"
    "ZenGin/Gothic_II_Addon/API/zCollisionObject.h"
    "ZenGin/Gothic_II_Addon/API/zCollisionObjectMisc.h"
    "ZenGin/Gothic_II_Addon/API/zConsole.h"
    "ZenGin/Gothic_II_Addon/API/zContainer.h"
    "ZenGin/Gothic_II_Addon/API/zConvexCollision.h"
    "ZenGin/Gothic_II_Addon/API/zCsCamera.h"
    "ZenGin/Gothic_II_Addon/API/zDisk.h"
    "ZenGin/Gothic_II_Addon/API/zDisk_Vdfs.h"
    "ZenGin/Gothic_II_Addon/API/zEigen.h"
    "ZenGin/Gothic_II_Addon/API/zEngine.h"
    "ZenGin/Gothic_II_Addon/API/zError.h"
    "ZenGin/Gothic_II_Addon/API/zEventMan.h"
    "ZenGin/Gothic_II_Addon/API/zFft.h"
    "ZenGin/Gothic_II_Addon/API/zFile3d.h"
    "ZenGin/Gothic_II_Addon/API/zFileCsv.h"
    "ZenGin/Gothic_II_Addon/API/zFlash.h"
    "ZenGin/Gothic_II_Addon/API/zFonts.h"
    "ZenGin/Gothic_II_Addon/API/zGameInfo.h"
    "ZenGin/Gothic_II_Addon/API/zGlobal.h"
    "ZenGin/Gothic_II_Addon/API/zGrafix.h"
    "ZenGin/Gothic_II_Addon/API/zIncDialogs.h"
    "ZenGin/Gothic_II_Addon/API/zIncViews.h"
    "ZenGin/Gothic_II_Addon/API/zInput.h"
    "ZenGin/Gothic_II_Addon/API/zInput_Const.h"
    "ZenGin/Gothic_II_Addon/API/zInput_Win32.h"
    "ZenGin/Gothic_II_Addon/API/zKbSplines.h"
    "ZenGin/Gothic_II_Addon/API/zLensflare.h"
    "ZenGin/Gothic_II_Addon/API/zList.h"
    "ZenGin/Gothic_II_Addon/API/zMacro.h"
    "ZenGin/Gothic_II_Addon/API/zMaterial.h"
    "ZenGin/Gothic_II_Addon/API/zMathTools.h"
    "ZenGin/Gothic_II_Addon/API/zMemory_Base.h"
    "ZenGin/Gothic_II_Addon/API/zMemory_Generic.h"
    "ZenGin/Gothic_II_Addon/API/zMemory_Win32.h"
    "ZenGin/Gothic_II_Addon/API/zMemPool.h"
    "ZenGin/Gothic_II_Addon/API/zMenu.h"
    "ZenGin/Gothic_II_Addon/API/zMenu_Item.h"
    "ZenGin/Gothic_II_Addon/API/zModel.h"
    "ZenGin/Gothic_II_Addon/API/zMorphMesh.h"
    "ZenGin/Gothic_II_Addon/API/zMoving.h"
    "ZenGin/Gothic_II_Addon/API/zMusic.h"
    "ZenGin/Gothic_II_Addon/API/zMusic_Dm.h"
    "ZenGin/Gothic_II_Addon/API/zMusicCtrl.h"
    "ZenGin/Gothic_II_Addon/API/zNet.h"
    "ZenGin/Gothic_II_Addon/API/zNet_Win32.h"
    "ZenGin/Gothic_II_Addon/API/zNetEventMan.h"
    "ZenGin/Gothic_II_Addon/API/zNetHost.h"
    "ZenGin/Gothic_II_Addon/API/zNetManager.h"
    "ZenGin/Gothic_II_Addon/API/zNetMessage.h"
    "ZenGin/Gothic_II_Addon/API/zNetSession.h"
    "ZenGin/Gothic_II_Addon/API/zNetTypes.h"
    "ZenGin/Gothic_II_Addon/API/zNetVersion.h"
    "ZenGin/Gothic_II_Addon/API/zNetVobControl.h"
    "ZenGin/Gothic_II_Addon/API/zObject.h"
    "ZenGin/Gothic_II_Addon/API/zOption.h"
    "ZenGin/Gothic_II_Addon/API/zParser.h"
    "ZenGin/Gothic_II_Addon/API/zParser_Const.h"
    "ZenGin/Gothic_II_Addon/API/zParser_Symbol.h"
    "ZenGin/Gothic_II_Addon/API/zParticle.h"
    "ZenGin/Gothic_II_Addon/API/zPhysics.h"
    "ZenGin/Gothic_II_Addon/API/zPlayerGroup.h"
    "ZenGin/Gothic_II_Addon/API/zPlayerInfo.h"
    "ZenGin/Gothic_II_Addon/API/zPluecker.h"
    "ZenGin/Gothic_II_Addon/API/zPolyPool.h"
    "ZenGin/Gothic_II_Addon/API/zPolyStrip.h"
    "ZenGin/Gothic_II_Addon/API/zPrime.h"
    "ZenGin/Gothic_II_Addon/API/zProgMesh.h"
    "ZenGin/Gothic_II_Addon/API/zPrototypes.h"
    "ZenGin/Gothic_II_Addon/API/zQueue.h"
    "ZenGin/Gothic_II_Addon/API/zRayCache.h"
    "ZenGin/Gothic_II_Addon/API/zRenderer.h"
    "ZenGin/Gothic_II_Addon/API/zRenderLightContainer.h"
    "ZenGin/Gothic_II_Addon/API/zRenderManager.h"
    "ZenGin/Gothic_II_Addon/API/zResource.h"
    "ZenGin/Gothic_II_Addon/API/zRndD3d.h"
    "ZenGin/Gothic_II_Addon/API/zRndD3d_Init.h"
    "ZenGin/Gothic_II_Addon/API/zScanDir.h"
    "ZenGin/Gothic_II_Addon/API/zSession.h"
    "ZenGin/Gothic_II_Addon/API/zSky.h"
    "ZenGin/Gothic_II_Addon/API/zSky_Outdoor.h"
    "ZenGin/Gothic_II_Addon/API/zSndMss.h"
    "ZenGin/Gothic_II_Addon/API/zSound.h"
    "ZenGin/Gothic_II_Addon/API/zSoundMan.h"
    "ZenGin/Gothic_II_Addon/API/zSparseArray.h"
    "ZenGin/Gothic_II_Addon/API/zSpyTypes.h"
    "ZenGin/Gothic_II_Addon/API/zString.h"
    "ZenGin/Gothic_II_Addon/API/zSysInfo.h"
    "ZenGin/Gothic_II_Addon/API/zTexConvert.h"
    "ZenGin/Gothic_II_Addon/API/zTexture.h"
    "ZenGin/Gothic_II_Addon/API/zThread.h"
    "ZenGin/Gothic_II_Addon/API/zTools.h"
    "ZenGin/Gothic_II_Addon/API/zTypes.h"
    "ZenGin/Gothic_II_Addon/API/zTypes3d.h"
    "ZenGin/Gothic_II_Addon/API/zVertexBuffer.h"
    "ZenGin/Gothic_II_Addon/API/zVertexTransform.h"
    "ZenGin/Gothic_II_Addon/API/zVideo_Win32.h"
    "ZenGin/Gothic_II_Addon/API/zVideoPlayer.h"
    "ZenGin/Gothic_II_Addon/API/zView.h"
    "ZenGin/Gothic_II_Addon/API/zViewBase.h"
    "ZenGin/Gothic_II_Addon/API/zViewDialog.h"
    "ZenGin/Gothic_II_Addon/API/zViewDialogChoice.h"
    "ZenGin/Gothic_II_Addon/API/zViewDraw.h"
    "ZenGin/Gothic_II_Addon/API/zViewFx.h"
    "ZenGin/Gothic_II_Addon/API/zViewObject.h"
    "ZenGin/Gothic_II_Addon/API/zViewPrint.h"
    "ZenGin/Gothic_II_Addon/API/zViewPrint_Font.h"
    "ZenGin/Gothic_II_Addon/API/zViewProgressbar.h"
    "ZenGin/Gothic_II_Addon/API/zViews.h"
    "ZenGin/Gothic_II_Addon/API/zViewStatusbar.h"
    "ZenGin/Gothic_II_Addon/API/zVisual.h"
    "ZenGin/Gothic_II_Addon/API/zVob.h"
    "ZenGin/Gothic_II_Addon/API/zVobIdentifier.h"
    "ZenGin/Gothic_II_Addon/API/zVobMisc.h"
    "ZenGin/Gothic_II_Addon/API/zWaynet.h"
    "ZenGin/Gothic_II_Addon/API/zWorld.h"
    "ZenGin/Gothic_II_Addon/API/zWorldInfo.h"
    "ZenGin/Gothic_II_Addon/API/zZone.h"
)
source_group("Engine SDK\\4. Gothic II NoTR\\Headers\\zEngine" FILES ${Engine_SDK__4._Gothic_II_NoTR__Headers__zEngine})

set(Engine_SDK__4._Gothic_II_NoTR__Overloads
    "ZenGin/Gothic_II_Addon/ClassDeclarators_G2A.cpp"
    "ZenGin/Gothic_II_Addon/Globals_G2A.cpp"
    "ZenGin/Gothic_II_Addon/Statics_G2A.cpp"
    "ZenGin/Gothic_II_Addon/VFTable_G2A.h"
)
source_group("Engine SDK\\4. Gothic II NoTR\\Overloads" FILES ${Engine_SDK__4._Gothic_II_NoTR__Overloads})

set(Engine_SDK__User_API__oGame__A
    "ZenGin/Gothic_UserAPI/oCAIArrow.inl"
    "ZenGin/Gothic_UserAPI/oCAIArrowBase.inl"
    "ZenGin/Gothic_UserAPI/oCAICamera.inl"
    "ZenGin/Gothic_UserAPI/oCAIDrop.inl"
    "ZenGin/Gothic_UserAPI/oCAIHuman.inl"
    "ZenGin/Gothic_UserAPI/oCAIHuman_Stand.inl"
    "ZenGin/Gothic_UserAPI/oCAISound.inl"
    "ZenGin/Gothic_UserAPI/oCAIVobMove.inl"
    "ZenGin/Gothic_UserAPI/oCAIVobMoveTorch.inl"
    "ZenGin/Gothic_UserAPI/oCAniCtrl_Human.inl"
    "ZenGin/Gothic_UserAPI/oCAniCtrl_Human_oTComboInfo.inl"
    "ZenGin/Gothic_UserAPI/oCAskBox.inl"
)
source_group("Engine SDK\\User API\\oGame\\A" FILES ${Engine_SDK__User_API__oGame__A})

set(Engine_SDK__User_API__oGame__B
    "ZenGin/Gothic_UserAPI/oCBarrier.inl"
    "ZenGin/Gothic_UserAPI/oCBinkPlayer.inl"
)
source_group("Engine SDK\\User API\\oGame\\B" FILES ${Engine_SDK__User_API__oGame__B})

set(Engine_SDK__User_API__oGame__C
    "ZenGin/Gothic_UserAPI/oCCollObjectCharacter.inl"
    "ZenGin/Gothic_UserAPI/oCCSManager.inl"
    "ZenGin/Gothic_UserAPI/oCCSPlayer.inl"
    "ZenGin/Gothic_UserAPI/oCCSProps.inl"
    "ZenGin/Gothic_UserAPI/oCCSTrigger.inl"
)
source_group("Engine SDK\\User API\\oGame\\C" FILES ${Engine_SDK__User_API__oGame__C})

set(Engine_SDK__User_API__oGame__D
    "ZenGin/Gothic_UserAPI/oCDoc.inl"
    "ZenGin/Gothic_UserAPI/oCDocumentManager.inl"
    "ZenGin/Gothic_UserAPI/oCDummyVobGenerator.inl"
)
source_group("Engine SDK\\User API\\oGame\\D" FILES ${Engine_SDK__User_API__oGame__D})

set(Engine_SDK__User_API__oGame__E
    "ZenGin/Gothic_UserAPI/oCEmitterKey.inl"
)
source_group("Engine SDK\\User API\\oGame\\E" FILES ${Engine_SDK__User_API__oGame__E})

set(Engine_SDK__User_API__oGame__G
    "ZenGin/Gothic_UserAPI/CGameManager.inl"
    "ZenGin/Gothic_UserAPI/oCGame.inl"
    "ZenGin/Gothic_UserAPI/oCGame_TObjectRoutine.inl"
    "ZenGin/Gothic_UserAPI/oCGameInfo.inl"
    "ZenGin/Gothic_UserAPI/oCGuilds.inl"
)
source_group("Engine SDK\\User API\\oGame\\G" FILES ${Engine_SDK__User_API__oGame__G})

set(Engine_SDK__User_API__oGame__H
    "ZenGin/Gothic_UserAPI/oCHelp.inl"
    "ZenGin/Gothic_UserAPI/oCHelpScreen.inl"
)
source_group("Engine SDK\\User API\\oGame\\H" FILES ${Engine_SDK__User_API__oGame__H})

set(Engine_SDK__User_API__oGame__I
    "ZenGin/Gothic_UserAPI/oCInfo.inl"
    "ZenGin/Gothic_UserAPI/oCInfo_Tpd.inl"
    "ZenGin/Gothic_UserAPI/oCInfoChoice.inl"
    "ZenGin/Gothic_UserAPI/oCInfoManager.inl"
    "ZenGin/Gothic_UserAPI/oCInformationManager.inl"
    "ZenGin/Gothic_UserAPI/oCItem.inl"
    "ZenGin/Gothic_UserAPI/oCItemContainer.inl"
    "ZenGin/Gothic_UserAPI/oCItemReactModule.inl"
)
source_group("Engine SDK\\User API\\oGame\\I" FILES ${Engine_SDK__User_API__oGame__I})

set(Engine_SDK__User_API__oGame__L
    "ZenGin/Gothic_UserAPI/oCLogEntry.inl"
    "ZenGin/Gothic_UserAPI/oCLogManager.inl"
    "ZenGin/Gothic_UserAPI/oCLogScreen.inl"
    "ZenGin/Gothic_UserAPI/oCLogTopic.inl"
)
source_group("Engine SDK\\User API\\oGame\\L" FILES ${Engine_SDK__User_API__oGame__L})

set(Engine_SDK__User_API__oGame__M
    "ZenGin/Gothic_UserAPI/oCMag_Book.inl"
    "ZenGin/Gothic_UserAPI/oCMagFrontier.inl"
    "ZenGin/Gothic_UserAPI/oCMapScreen.inl"
    "ZenGin/Gothic_UserAPI/oCMenu_ChgKeys.inl"
    "ZenGin/Gothic_UserAPI/oCMenu_Help.inl"
    "ZenGin/Gothic_UserAPI/oCMenu_Log.inl"
    "ZenGin/Gothic_UserAPI/oCMenu_Main.inl"
    "ZenGin/Gothic_UserAPI/oCMenu_Status.inl"
    "ZenGin/Gothic_UserAPI/oCMenuSavegame.inl"
    "ZenGin/Gothic_UserAPI/oCMission.inl"
    "ZenGin/Gothic_UserAPI/oCMission_TMisStatus.inl"
    "ZenGin/Gothic_UserAPI/oCMissionManager.inl"
    "ZenGin/Gothic_UserAPI/oCMOB.inl"
    "ZenGin/Gothic_UserAPI/oCMobBed.inl"
    "ZenGin/Gothic_UserAPI/oCMobContainer.inl"
    "ZenGin/Gothic_UserAPI/oCMobDoor.inl"
    "ZenGin/Gothic_UserAPI/oCMobFire.inl"
    "ZenGin/Gothic_UserAPI/oCMobInter.inl"
    "ZenGin/Gothic_UserAPI/oCMobItemSlot.inl"
    "ZenGin/Gothic_UserAPI/oCMobLadder.inl"
    "ZenGin/Gothic_UserAPI/oCMobLockable.inl"
    "ZenGin/Gothic_UserAPI/oCMobMsg.inl"
    "ZenGin/Gothic_UserAPI/oCMobSwitch.inl"
    "ZenGin/Gothic_UserAPI/oCMobWheel.inl"
    "ZenGin/Gothic_UserAPI/oCMsgAttack.inl"
    "ZenGin/Gothic_UserAPI/oCMsgConversation.inl"
    "ZenGin/Gothic_UserAPI/oCMsgDamage.inl"
    "ZenGin/Gothic_UserAPI/oCMsgMagic.inl"
    "ZenGin/Gothic_UserAPI/oCMsgManipulate.inl"
    "ZenGin/Gothic_UserAPI/oCMsgMovement.inl"
    "ZenGin/Gothic_UserAPI/oCMsgRequest.inl"
    "ZenGin/Gothic_UserAPI/oCMsgState.inl"
    "ZenGin/Gothic_UserAPI/oCMsgUseItem.inl"
    "ZenGin/Gothic_UserAPI/oCMsgWeapon.inl"
    "ZenGin/Gothic_UserAPI/oSMenuInfoArmor.inl"
    "ZenGin/Gothic_UserAPI/oSMenuInfoAttribute.inl"
    "ZenGin/Gothic_UserAPI/oSMenuInfoTalent.inl"
    "ZenGin/Gothic_UserAPI/oSMenuKey.inl"
    "ZenGin/Gothic_UserAPI/TMobOptPos.inl"
)
source_group("Engine SDK\\User API\\oGame\\M" FILES ${Engine_SDK__User_API__oGame__M})

set(Engine_SDK__User_API__oGame__N
    "ZenGin/Gothic_UserAPI/oCNews.inl"
    "ZenGin/Gothic_UserAPI/oCNewsManager.inl"
    "ZenGin/Gothic_UserAPI/oCNewsMemory.inl"
    "ZenGin/Gothic_UserAPI/oCNpc.inl"
    "ZenGin/Gothic_UserAPI/oCNpc_oCNpcTimedOverlay.inl"
    "ZenGin/Gothic_UserAPI/oCNpc_oSDirectionInfo.inl"
    "ZenGin/Gothic_UserAPI/oCNpc_oTRobustTrace.inl"
    "ZenGin/Gothic_UserAPI/oCNpc_States.inl"
    "ZenGin/Gothic_UserAPI/oCNpc_TActiveInfo.inl"
    "ZenGin/Gothic_UserAPI/oCNpcContainer.inl"
    "ZenGin/Gothic_UserAPI/oCNpcFocus.inl"
    "ZenGin/Gothic_UserAPI/oCNpcInventory.inl"
    "ZenGin/Gothic_UserAPI/oCNpcMessage.inl"
    "ZenGin/Gothic_UserAPI/oCNpcTalent.inl"
    "ZenGin/Gothic_UserAPI/TNpcAIState.inl"
    "ZenGin/Gothic_UserAPI/TNpcPerc.inl"
    "ZenGin/Gothic_UserAPI/TNpcSlot.inl"
)
source_group("Engine SDK\\User API\\oGame\\N" FILES ${Engine_SDK__User_API__oGame__N})

set(Engine_SDK__User_API__oGame__O
    "ZenGin/Gothic_UserAPI/oCObjectFactory.inl"
    "ZenGin/Gothic_UserAPI/oCObjectGenerator.inl"
)
source_group("Engine SDK\\User API\\oGame\\O" FILES ${Engine_SDK__User_API__oGame__O})

set(Engine_SDK__User_API__oGame__P
    "ZenGin/Gothic_UserAPI/oCParticleControl.inl"
    "ZenGin/Gothic_UserAPI/oCPlayerInfo.inl"
    "ZenGin/Gothic_UserAPI/oCPortalRoom.inl"
    "ZenGin/Gothic_UserAPI/oCPortalRoomManager.inl"
)
source_group("Engine SDK\\User API\\oGame\\P" FILES ${Engine_SDK__User_API__oGame__P})

set(Engine_SDK__User_API__oGame__R
    "ZenGin/Gothic_UserAPI/oCRtnCutscene.inl"
    "ZenGin/Gothic_UserAPI/oCRtnEntry.inl"
    "ZenGin/Gothic_UserAPI/oCRtnManager.inl"
    "ZenGin/Gothic_UserAPI/oCRtnManager_TRtn_WayBox.inl"
    "ZenGin/Gothic_UserAPI/oCRtnManager_TRtn_WayBoxLimit.inl"
)
source_group("Engine SDK\\User API\\oGame\\R" FILES ${Engine_SDK__User_API__oGame__R})

set(Engine_SDK__User_API__oGame__S
    "ZenGin/Gothic_UserAPI/oCSavegameInfo.inl"
    "ZenGin/Gothic_UserAPI/oCSavegameManager.inl"
    "ZenGin/Gothic_UserAPI/oCSkyControler_Barrier.inl"
    "ZenGin/Gothic_UserAPI/oCSpawnManager.inl"
    "ZenGin/Gothic_UserAPI/oCSpell.inl"
    "ZenGin/Gothic_UserAPI/oCStatusScreen.inl"
    "ZenGin/Gothic_UserAPI/oCStealContainer.inl"
    "ZenGin/Gothic_UserAPI/oCSVM.inl"
    "ZenGin/Gothic_UserAPI/oCSVMManager.inl"
    "ZenGin/Gothic_UserAPI/oCSystemInfo.inl"
)
source_group("Engine SDK\\User API\\oGame\\S" FILES ${Engine_SDK__User_API__oGame__S})

set(Engine_SDK__User_API__oGame__T
    "ZenGin/Gothic_UserAPI/oCText.inl"
    "ZenGin/Gothic_UserAPI/oCTouchDamage.inl"
    "ZenGin/Gothic_UserAPI/oCTradeManager.inl"
    "ZenGin/Gothic_UserAPI/oCTrajectory.inl"
    "ZenGin/Gothic_UserAPI/oCTriggerChangeLevel.inl"
    "ZenGin/Gothic_UserAPI/oCTriggerScript.inl"
)
source_group("Engine SDK\\User API\\oGame\\T" FILES ${Engine_SDK__User_API__oGame__T})

set(Engine_SDK__User_API__oGame__V
    "ZenGin/Gothic_UserAPI/oCViewDialogInventory.inl"
    "ZenGin/Gothic_UserAPI/oCViewDialogItem.inl"
    "ZenGin/Gothic_UserAPI/oCViewDialogItemContainer.inl"
    "ZenGin/Gothic_UserAPI/oCViewDialogStealContainer.inl"
    "ZenGin/Gothic_UserAPI/oCViewDialogTrade.inl"
    "ZenGin/Gothic_UserAPI/oCViewDocument.inl"
    "ZenGin/Gothic_UserAPI/oCViewDocumentMap.inl"
    "ZenGin/Gothic_UserAPI/oCViewGothicProgressBar.inl"
    "ZenGin/Gothic_UserAPI/oCViewStatusBar.inl"
    "ZenGin/Gothic_UserAPI/oCVisFX_MultiTarget.inl"
    "ZenGin/Gothic_UserAPI/oCVisualFX.inl"
    "ZenGin/Gothic_UserAPI/oCVisualFXAI.inl"
    "ZenGin/Gothic_UserAPI/oCVob.inl"
)
source_group("Engine SDK\\User API\\oGame\\V" FILES ${Engine_SDK__User_API__oGame__V})

set(Engine_SDK__User_API__oGame__W
    "ZenGin/Gothic_UserAPI/oCWay.inl"
    "ZenGin/Gothic_UserAPI/oCWaypoint.inl"
    "ZenGin/Gothic_UserAPI/oCWorld.inl"
    "ZenGin/Gothic_UserAPI/oCWorldTimer.inl"
)
source_group("Engine SDK\\User API\\oGame\\W" FILES ${Engine_SDK__User_API__oGame__W})

set(Engine_SDK__User_API__oGame__Z
    "ZenGin/Gothic_UserAPI/oCZoneMusic.inl"
    "ZenGin/Gothic_UserAPI/oCZoneMusicDefault.inl"
    "ZenGin/Gothic_UserAPI/oCZoneMusicList.inl"
)
source_group("Engine SDK\\User API\\oGame\\Z" FILES ${Engine_SDK__User_API__oGame__Z})

set(Engine_SDK__User_API__zEngine__A
    "ZenGin/Gothic_UserAPI/zCAccount.inl"
    "ZenGin/Gothic_UserAPI/zCActiveSnd.inl"
    "ZenGin/Gothic_UserAPI/zCAIBase.inl"
    "ZenGin/Gothic_UserAPI/zCAIBaseSound.inl"
    "ZenGin/Gothic_UserAPI/zCAICamera.inl"
    "ZenGin/Gothic_UserAPI/zCAIPlayer.inl"
    "ZenGin/Gothic_UserAPI/zCAIPlayer_zTBloodVobTrack.inl"
    "ZenGin/Gothic_UserAPI/zCAIPlayer_zTConfig.inl"
    "ZenGin/Gothic_UserAPI/zCAIPlayer_zTLedgeInfo.inl"
    "ZenGin/Gothic_UserAPI/zCArchiver.inl"
    "ZenGin/Gothic_UserAPI/zCArchiver_zTChunkRecord.inl"
    "ZenGin/Gothic_UserAPI/zCArchiverBinSafe.inl"
    "ZenGin/Gothic_UserAPI/zCArchiverFactory.inl"
    "ZenGin/Gothic_UserAPI/zCArchiverGeneric.inl"
    "ZenGin/Gothic_UserAPI/zCArchiverGeneric_zTWriteObjectEntry.inl"
    "ZenGin/Gothic_UserAPI/zCArray.inl"
    "ZenGin/Gothic_UserAPI/zCArrayAdapt.inl"
    "ZenGin/Gothic_UserAPI/zCArraySort.inl"
    "ZenGin/Gothic_UserAPI/zTAniAttachment.inl"
)
source_group("Engine SDK\\User API\\zEngine\\A" FILES ${Engine_SDK__User_API__zEngine__A})

set(Engine_SDK__User_API__zEngine__B
    "ZenGin/Gothic_UserAPI/zCBBox3DSorterBase.inl"
    "ZenGin/Gothic_UserAPI/zCBBox3DSorterBase_zTBoxSortHandle.inl"
    "ZenGin/Gothic_UserAPI/zCBBox3DSorterBase_zTNode.inl"
    "ZenGin/Gothic_UserAPI/zCBinkPlayer.inl"
    "ZenGin/Gothic_UserAPI/zCBspBase.inl"
    "ZenGin/Gothic_UserAPI/zCBspLeaf.inl"
    "ZenGin/Gothic_UserAPI/zCBspNode.inl"
    "ZenGin/Gothic_UserAPI/zCBspSector.inl"
    "ZenGin/Gothic_UserAPI/zCBspSector_zTPortalInfo.inl"
    "ZenGin/Gothic_UserAPI/zCBspTree.inl"
    "ZenGin/Gothic_UserAPI/zCBuffer.inl"
    "ZenGin/Gothic_UserAPI/zCBuildPortal.inl"
    "ZenGin/Gothic_UserAPI/zTBBox2D.inl"
    "ZenGin/Gothic_UserAPI/zTBBox3D.inl"
    "ZenGin/Gothic_UserAPI/zTBCylinder3D.inl"
    "ZenGin/Gothic_UserAPI/zTBSphere3D.inl"
)
source_group("Engine SDK\\User API\\zEngine\\B" FILES ${Engine_SDK__User_API__zEngine__B})

set(Engine_SDK__User_API__zEngine__C
    "ZenGin/Gothic_UserAPI/zCCacheBase.inl"
    "ZenGin/Gothic_UserAPI/zCCacheData.inl"
    "ZenGin/Gothic_UserAPI/zCCacheIndex.inl"
    "ZenGin/Gothic_UserAPI/zCCallback.inl"
    "ZenGin/Gothic_UserAPI/zCCamera.inl"
    "ZenGin/Gothic_UserAPI/zCCamera_zTCamVertSimple.inl"
    "ZenGin/Gothic_UserAPI/zCCamTrj_KeyFrame.inl"
    "ZenGin/Gothic_UserAPI/zCCBspNode.inl"
    "ZenGin/Gothic_UserAPI/zCCBspTree.inl"
    "ZenGin/Gothic_UserAPI/zCChecksum.inl"
    "ZenGin/Gothic_UserAPI/zCChecksum16.inl"
    "ZenGin/Gothic_UserAPI/zCCircleArc3D.inl"
    "ZenGin/Gothic_UserAPI/zCClassDef.inl"
    "ZenGin/Gothic_UserAPI/zCCodeMaster.inl"
    "ZenGin/Gothic_UserAPI/zCCollisionDetector.inl"
    "ZenGin/Gothic_UserAPI/zCCollisionObject.inl"
    "ZenGin/Gothic_UserAPI/zCCollisionObjectDef.inl"
    "ZenGin/Gothic_UserAPI/zCCollisionReport.inl"
    "ZenGin/Gothic_UserAPI/zCCollObjectBase.inl"
    "ZenGin/Gothic_UserAPI/zCCollObjectBoxBlocker.inl"
    "ZenGin/Gothic_UserAPI/zCCollObjectBoxPassThrough.inl"
    "ZenGin/Gothic_UserAPI/zCCollObjectCharacter.inl"
    "ZenGin/Gothic_UserAPI/zCCollObjectCharacter_zTConfig.inl"
    "ZenGin/Gothic_UserAPI/zCCollObjectCharacter_zTInterferenceReport.inl"
    "ZenGin/Gothic_UserAPI/zCCollObjectCharacter_zTSpatialState.inl"
    "ZenGin/Gothic_UserAPI/zCCollObjectComplex.inl"
    "ZenGin/Gothic_UserAPI/zCCollObjectLevelPolys.inl"
    "ZenGin/Gothic_UserAPI/zCCollObjectPoint.inl"
    "ZenGin/Gothic_UserAPI/zCCollObjectProjectile.inl"
    "ZenGin/Gothic_UserAPI/zCCollObjectUndef.inl"
    "ZenGin/Gothic_UserAPI/zCConDat.inl"
    "ZenGin/Gothic_UserAPI/zCConsole.inl"
    "ZenGin/Gothic_UserAPI/zCConvexCollisionCache.inl"
    "ZenGin/Gothic_UserAPI/zCConvexPrimitive.inl"
    "ZenGin/Gothic_UserAPI/zCConvexPrimitiveEllipsoid.inl"
    "ZenGin/Gothic_UserAPI/zCConvexPrimitiveScaleTrans.inl"
    "ZenGin/Gothic_UserAPI/zCConvexPrimitiveUnitSphere.inl"
    "ZenGin/Gothic_UserAPI/zCCriticalSection.inl"
    "ZenGin/Gothic_UserAPI/zCCSAtomicBlock.inl"
    "ZenGin/Gothic_UserAPI/zCCSBlock.inl"
    "ZenGin/Gothic_UserAPI/zCCSBlockBase.inl"
    "ZenGin/Gothic_UserAPI/zCCSBlockPosition.inl"
    "ZenGin/Gothic_UserAPI/zCCSCamera.inl"
    "ZenGin/Gothic_UserAPI/zCCSCamera_EventMsg.inl"
    "ZenGin/Gothic_UserAPI/zCCSCamera_EventMsgActivate.inl"
    "ZenGin/Gothic_UserAPI/zCCSCutsceneContext.inl"
    "ZenGin/Gothic_UserAPI/zCCSDebugger.inl"
    "ZenGin/Gothic_UserAPI/zCCSLib.inl"
    "ZenGin/Gothic_UserAPI/zCCSManager.inl"
    "ZenGin/Gothic_UserAPI/zCCSPlayer.inl"
    "ZenGin/Gothic_UserAPI/zCCSProps.inl"
    "ZenGin/Gothic_UserAPI/zCCSSyncBlock.inl"
    "ZenGin/Gothic_UserAPI/zCCubicPolynomial.inl"
    "ZenGin/Gothic_UserAPI/zCCutscene.inl"
    "ZenGin/Gothic_UserAPI/zSCtrlValue.inl"
    "ZenGin/Gothic_UserAPI/zTCSBlockPosition.inl"
)
source_group("Engine SDK\\User API\\zEngine\\C" FILES ${Engine_SDK__User_API__zEngine__C})

set(Engine_SDK__User_API__zEngine__D
    "ZenGin/Gothic_UserAPI/zCDecal.inl"
    "ZenGin/Gothic_UserAPI/zCDrvProperties_MSS.inl"
    "ZenGin/Gothic_UserAPI/zCDXTCCache_D3D.inl"
    "ZenGin/Gothic_UserAPI/zCDXTCCache_D3D_zD3D_Entry.inl"
    "ZenGin/Gothic_UserAPI/zCDXTCCache_D3D_zD3D_SlotHeader.inl"
    "ZenGin/Gothic_UserAPI/zD3D_alphaPoly.inl"
    "ZenGin/Gothic_UserAPI/zD3D_alphaPoly_zD3D_vertexFormat.inl"
    "ZenGin/Gothic_UserAPI/zD3D_savedTexture.inl"
    "ZenGin/Gothic_UserAPI/zD3DTEX.inl"
    "ZenGin/Gothic_UserAPI/zDATE.inl"
)
source_group("Engine SDK\\User API\\zEngine\\D" FILES ${Engine_SDK__User_API__zEngine__D})

set(Engine_SDK__User_API__zEngine__E
    "ZenGin/Gothic_UserAPI/zCEarthquake.inl"
    "ZenGin/Gothic_UserAPI/zCEffect.inl"
    "ZenGin/Gothic_UserAPI/zCEventCommon.inl"
    "ZenGin/Gothic_UserAPI/zCEventCore.inl"
    "ZenGin/Gothic_UserAPI/zCEventManager.inl"
    "ZenGin/Gothic_UserAPI/zCEventMessage.inl"
    "ZenGin/Gothic_UserAPI/zCEventMover.inl"
    "ZenGin/Gothic_UserAPI/zCEventMusicControler.inl"
    "ZenGin/Gothic_UserAPI/zCEventScreenFX.inl"
    "ZenGin/Gothic_UserAPI/zCEvMsgCutscene.inl"
    "ZenGin/Gothic_UserAPI/zERROR.inl"
    "ZenGin/Gothic_UserAPI/zSEvasionSearchDesc.inl"
    "ZenGin/Gothic_UserAPI/zSEvasionSearchDesc_zSEvasionSearchOptions.inl"
    "ZenGin/Gothic_UserAPI/zTEngineStats.inl"
    "ZenGin/Gothic_UserAPI/zTErrorMessage.inl"
)
source_group("Engine SDK\\User API\\zEngine\\E" FILES ${Engine_SDK__User_API__zEngine__E})

set(Engine_SDK__User_API__zEngine__F
    "ZenGin/Gothic_UserAPI/zCFFT.inl"
    "ZenGin/Gothic_UserAPI/zCFile3DS.inl"
    "ZenGin/Gothic_UserAPI/zCFileBIN.inl"
    "ZenGin/Gothic_UserAPI/zCFileCSV.inl"
    "ZenGin/Gothic_UserAPI/zCFileQuakeBSP.inl"
    "ZenGin/Gothic_UserAPI/zCFlash.inl"
    "ZenGin/Gothic_UserAPI/zCFlash_zCBolt.inl"
    "ZenGin/Gothic_UserAPI/zCFont.inl"
    "ZenGin/Gothic_UserAPI/zCFontMan.inl"
    "ZenGin/Gothic_UserAPI/zCFPUControler.inl"
    "ZenGin/Gothic_UserAPI/zFILE.inl"
    "ZenGin/Gothic_UserAPI/zFILE_FILE.inl"
    "ZenGin/Gothic_UserAPI/zFILE_INFO.inl"
    "ZenGin/Gothic_UserAPI/zFILE_STATS.inl"
    "ZenGin/Gothic_UserAPI/zFILE_VDFS.inl"
)
source_group("Engine SDK\\User API\\zEngine\\F" FILES ${Engine_SDK__User_API__zEngine__F})

set(Engine_SDK__User_API__zEngine__G
    "ZenGin/Gothic_UserAPI/zCGameInfo.inl"
    "ZenGin/Gothic_UserAPI/zCGfx.inl"
)
source_group("Engine SDK\\User API\\zEngine\\G" FILES ${Engine_SDK__User_API__zEngine__G})

set(Engine_SDK__User_API__zEngine__H
    "ZenGin/Gothic_UserAPI/zCHandleManager.inl"
)
source_group("Engine SDK\\User API\\zEngine\\H" FILES ${Engine_SDK__User_API__zEngine__H})

set(Engine_SDK__User_API__zEngine__I
    "ZenGin/Gothic_UserAPI/zCInput.inl"
    "ZenGin/Gothic_UserAPI/zCInput_Win32.inl"
    "ZenGin/Gothic_UserAPI/zCInputCallback.inl"
)
source_group("Engine SDK\\User API\\zEngine\\I" FILES ${Engine_SDK__User_API__zEngine__I})

set(Engine_SDK__User_API__zEngine__K
    "ZenGin/Gothic_UserAPI/zCKBSpline.inl"
    "ZenGin/Gothic_UserAPI/zSKeyMapping.inl"
)
source_group("Engine SDK\\User API\\zEngine\\K" FILES ${Engine_SDK__User_API__zEngine__K})

set(Engine_SDK__User_API__zEngine__L
    "ZenGin/Gothic_UserAPI/zCLensFlareFX.inl"
    "ZenGin/Gothic_UserAPI/zCLensFlareFX_zTVobLightLensFlare.inl"
    "ZenGin/Gothic_UserAPI/zCLightMap.inl"
    "ZenGin/Gothic_UserAPI/zCLightning.inl"
    "ZenGin/Gothic_UserAPI/zCLightning_zCBolt.inl"
    "ZenGin/Gothic_UserAPI/zCLine2D.inl"
    "ZenGin/Gothic_UserAPI/zCLineCache.inl"
    "ZenGin/Gothic_UserAPI/zCLineCache_zTCacheEntry.inl"
    "ZenGin/Gothic_UserAPI/zCList.inl"
    "ZenGin/Gothic_UserAPI/zCListSort.inl"
    "ZenGin/Gothic_UserAPI/zCLivingObjectsState.inl"
    "ZenGin/Gothic_UserAPI/zList.inl"
    "ZenGin/Gothic_UserAPI/zTLazyMesh.inl"
)
source_group("Engine SDK\\User API\\zEngine\\L" FILES ${Engine_SDK__User_API__zEngine__L})

set(Engine_SDK__User_API__zEngine__M
    "ZenGin/Gothic_UserAPI/zCMalloc.inl"
    "ZenGin/Gothic_UserAPI/zCMallocGeneric.inl"
    "ZenGin/Gothic_UserAPI/zCMallocWin32Debug.inl"
    "ZenGin/Gothic_UserAPI/zCMapDetailTexture.inl"
    "ZenGin/Gothic_UserAPI/zCMapDetailTexture_zTMapDetailTextureEntry.inl"
    "ZenGin/Gothic_UserAPI/zCMaterial.inl"
    "ZenGin/Gothic_UserAPI/zCMatrixStack.inl"
    "ZenGin/Gothic_UserAPI/zCMemPoolBase.inl"
    "ZenGin/Gothic_UserAPI/zCMemPoolBlock.inl"
    "ZenGin/Gothic_UserAPI/zCMenu.inl"
    "ZenGin/Gothic_UserAPI/zCMenuItem.inl"
    "ZenGin/Gothic_UserAPI/zCMenuItemButton.inl"
    "ZenGin/Gothic_UserAPI/zCMenuItemChoice.inl"
    "ZenGin/Gothic_UserAPI/zCMenuItemInput.inl"
    "ZenGin/Gothic_UserAPI/zCMenuItemList.inl"
    "ZenGin/Gothic_UserAPI/zCMenuItemSlider.inl"
    "ZenGin/Gothic_UserAPI/zCMenuItemText.inl"
    "ZenGin/Gothic_UserAPI/zCMesh.inl"
    "ZenGin/Gothic_UserAPI/zCMeshesPool.inl"
    "ZenGin/Gothic_UserAPI/zCMeshOctreeNode.inl"
    "ZenGin/Gothic_UserAPI/zCMeshPool.inl"
    "ZenGin/Gothic_UserAPI/zCMeshSoftSkin.inl"
    "ZenGin/Gothic_UserAPI/zCMeshSoftSkin_zTNodeWedgeNormal.inl"
    "ZenGin/Gothic_UserAPI/zCMeshSoftSkin_zTWeightEntry.inl"
    "ZenGin/Gothic_UserAPI/zCMessageFilter.inl"
    "ZenGin/Gothic_UserAPI/zCMgcEigenD.inl"
    "ZenGin/Gothic_UserAPI/zCModel.inl"
    "ZenGin/Gothic_UserAPI/zCModel_zTAniMeshLibEntry.inl"
    "ZenGin/Gothic_UserAPI/zCModel_zTMdl_NodeVobAttachment.inl"
    "ZenGin/Gothic_UserAPI/zCModel_zTMdl_StartedVobFX.inl"
    "ZenGin/Gothic_UserAPI/zCModel_zTMeshLibEntry.inl"
    "ZenGin/Gothic_UserAPI/zCModelAni.inl"
    "ZenGin/Gothic_UserAPI/zCModelAniActive.inl"
    "ZenGin/Gothic_UserAPI/zCModelAniEvent.inl"
    "ZenGin/Gothic_UserAPI/zCModelConvertFileHandler.inl"
    "ZenGin/Gothic_UserAPI/zCModelMeshLib.inl"
    "ZenGin/Gothic_UserAPI/zCModelMeshLib_zTNodeMesh.inl"
    "ZenGin/Gothic_UserAPI/zCModelNode.inl"
    "ZenGin/Gothic_UserAPI/zCModelNodeInst.inl"
    "ZenGin/Gothic_UserAPI/zCModelNodeInst_zTNodeAni.inl"
    "ZenGin/Gothic_UserAPI/zCModelPrototype.inl"
    "ZenGin/Gothic_UserAPI/zCModelTexAniState.inl"
    "ZenGin/Gothic_UserAPI/zCMorphMesh.inl"
    "ZenGin/Gothic_UserAPI/zCMorphMesh_zTMorphAniEntry.inl"
    "ZenGin/Gothic_UserAPI/zCMorphMesh_zTRandAni.inl"
    "ZenGin/Gothic_UserAPI/zCMorphMeshAni.inl"
    "ZenGin/Gothic_UserAPI/zCMorphMeshConvertFileHandler.inl"
    "ZenGin/Gothic_UserAPI/zCMorphMeshProto.inl"
    "ZenGin/Gothic_UserAPI/zCMovementTracker.inl"
    "ZenGin/Gothic_UserAPI/zCMover.inl"
    "ZenGin/Gothic_UserAPI/zCMover_zTMov_Keyframe.inl"
    "ZenGin/Gothic_UserAPI/zCMoverControler.inl"
    "ZenGin/Gothic_UserAPI/zCMoveRecorder.inl"
    "ZenGin/Gothic_UserAPI/zCMoveRecorder_zTMOR_Sample.inl"
    "ZenGin/Gothic_UserAPI/zCMusicControler.inl"
    "ZenGin/Gothic_UserAPI/zCMusicJingle.inl"
    "ZenGin/Gothic_UserAPI/zCMusicJingle_DM.inl"
    "ZenGin/Gothic_UserAPI/zCMusicSys_DirectMusic.inl"
    "ZenGin/Gothic_UserAPI/zCMusicSys_Dummy.inl"
    "ZenGin/Gothic_UserAPI/zCMusicSystem.inl"
    "ZenGin/Gothic_UserAPI/zCMusicTheme.inl"
    "ZenGin/Gothic_UserAPI/zCMusicTheme_DM.inl"
    "ZenGin/Gothic_UserAPI/zCMutex.inl"
    "ZenGin/Gothic_UserAPI/zMAT3.inl"
    "ZenGin/Gothic_UserAPI/zMAT4.inl"
    "ZenGin/Gothic_UserAPI/zTMallocProcessStats.inl"
    "ZenGin/Gothic_UserAPI/zTMallocStats.inl"
    "ZenGin/Gothic_UserAPI/zTMdl_AniSample.inl"
    "ZenGin/Gothic_UserAPI/zTMenuItemListElement.inl"
)
source_group("Engine SDK\\User API\\zEngine\\M" FILES ${Engine_SDK__User_API__zEngine__M})

set(Engine_SDK__User_API__zEngine__N
    "ZenGin/Gothic_UserAPI/zCNet.inl"
    "ZenGin/Gothic_UserAPI/zCNetEventManager.inl"
    "ZenGin/Gothic_UserAPI/zCNetHost.inl"
    "ZenGin/Gothic_UserAPI/zCNetManager.inl"
    "ZenGin/Gothic_UserAPI/zCNetMessage.inl"
    "ZenGin/Gothic_UserAPI/zCNetMessageQueue.inl"
    "ZenGin/Gothic_UserAPI/zCNetSession.inl"
    "ZenGin/Gothic_UserAPI/zCNetStatistics.inl"
    "ZenGin/Gothic_UserAPI/zCNetVersion.inl"
    "ZenGin/Gothic_UserAPI/zCNetVobControl.inl"
    "ZenGin/Gothic_UserAPI/zTNetAddress.inl"
    "ZenGin/Gothic_UserAPI/zTNetBufferInfo.inl"
    "ZenGin/Gothic_UserAPI/zTNetMessageHeader.inl"
    "ZenGin/Gothic_UserAPI/zTNetPacket.inl"
    "ZenGin/Gothic_UserAPI/zTNetStatistics.inl"
    "ZenGin/Gothic_UserAPI/zTNetTimeout.inl"
)
source_group("Engine SDK\\User API\\zEngine\\N" FILES ${Engine_SDK__User_API__zEngine__N})

set(Engine_SDK__User_API__zEngine__O
    "ZenGin/Gothic_UserAPI/zCOBBox3D.inl"
    "ZenGin/Gothic_UserAPI/zCOBBox3D_zTBoxSortHandle.inl"
    "ZenGin/Gothic_UserAPI/zCObject.inl"
    "ZenGin/Gothic_UserAPI/zCObjectFactory.inl"
    "ZenGin/Gothic_UserAPI/zCOLOR.inl"
    "ZenGin/Gothic_UserAPI/zComplex.inl"
    "ZenGin/Gothic_UserAPI/zCOption.inl"
    "ZenGin/Gothic_UserAPI/zCOptionEntry.inl"
    "ZenGin/Gothic_UserAPI/zCOptionSection.inl"
    "ZenGin/Gothic_UserAPI/zCOutdoorRainFX.inl"
    "ZenGin/Gothic_UserAPI/zCOutdoorRainFX_zSCacheElement.inl"
    "ZenGin/Gothic_UserAPI/zCOutdoorRainFX_zSParticle.inl"
)
source_group("Engine SDK\\User API\\zEngine\\O" FILES ${Engine_SDK__User_API__zEngine__O})

set(Engine_SDK__User_API__zEngine__P
    "ZenGin/Gothic_UserAPI/zCPar_DataStack.inl"
    "ZenGin/Gothic_UserAPI/zCPar_File.inl"
    "ZenGin/Gothic_UserAPI/zCPar_Stack.inl"
    "ZenGin/Gothic_UserAPI/zCPar_StringTable.inl"
    "ZenGin/Gothic_UserAPI/zCPar_Symbol.inl"
    "ZenGin/Gothic_UserAPI/zCPar_SymbolTable.inl"
    "ZenGin/Gothic_UserAPI/zCPar_TreeNode.inl"
    "ZenGin/Gothic_UserAPI/zCParser.inl"
    "ZenGin/Gothic_UserAPI/zCParticleEmitter.inl"
    "ZenGin/Gothic_UserAPI/zCParticleEmitterVars.inl"
    "ZenGin/Gothic_UserAPI/zCParticleFX.inl"
    "ZenGin/Gothic_UserAPI/zCParticleFX_zCStaticPfxList.inl"
    "ZenGin/Gothic_UserAPI/zCPatch.inl"
    "ZenGin/Gothic_UserAPI/zCPatchMap.inl"
    "ZenGin/Gothic_UserAPI/zCPathSearch.inl"
    "ZenGin/Gothic_UserAPI/zCPathSearch_zSPathSearchOptions.inl"
    "ZenGin/Gothic_UserAPI/zCPFXControler.inl"
    "ZenGin/Gothic_UserAPI/zCPlayerGroup.inl"
    "ZenGin/Gothic_UserAPI/zCPlayerInfo.inl"
    "ZenGin/Gothic_UserAPI/zCPluecker.inl"
    "ZenGin/Gothic_UserAPI/zCPolygon.inl"
    "ZenGin/Gothic_UserAPI/zCPolygon_TFlags.inl"
    "ZenGin/Gothic_UserAPI/zCPolygon_zSFeatureSavedValues.inl"
    "ZenGin/Gothic_UserAPI/zCPolygon_zSPolySavedValues.inl"
    "ZenGin/Gothic_UserAPI/zCPolygon_zSVertexSavedValues.inl"
    "ZenGin/Gothic_UserAPI/zCPolygonPool.inl"
    "ZenGin/Gothic_UserAPI/zCPolyStrip.inl"
    "ZenGin/Gothic_UserAPI/zCPortal.inl"
    "ZenGin/Gothic_UserAPI/zCPose.inl"
    "ZenGin/Gothic_UserAPI/zCPositionKey.inl"
    "ZenGin/Gothic_UserAPI/zCPQueue.inl"
    "ZenGin/Gothic_UserAPI/zCPrime.inl"
    "ZenGin/Gothic_UserAPI/zCProgMeshBuilder.inl"
    "ZenGin/Gothic_UserAPI/zCProgMeshConvertFileHandler.inl"
    "ZenGin/Gothic_UserAPI/zCProgMeshProto.inl"
    "ZenGin/Gothic_UserAPI/zCProgMeshProto_zCSubMesh.inl"
    "ZenGin/Gothic_UserAPI/zCProgMeshProto_zTLODParams.inl"
    "ZenGin/Gothic_UserAPI/zCProgMeshProto_zTLODRenderArgs.inl"
    "ZenGin/Gothic_UserAPI/zCProgMeshProto_zTLODRenderArgsSubMesh.inl"
    "ZenGin/Gothic_UserAPI/zPATH.inl"
    "ZenGin/Gothic_UserAPI/zSPathSearchResult.inl"
    "ZenGin/Gothic_UserAPI/zSPathSearchState.inl"
    "ZenGin/Gothic_UserAPI/zSPathSearchState_Internal.inl"
    "ZenGin/Gothic_UserAPI/zTPlane.inl"
    "ZenGin/Gothic_UserAPI/zTPMEdge.inl"
    "ZenGin/Gothic_UserAPI/zTPMTriangle.inl"
    "ZenGin/Gothic_UserAPI/zTPMTriangleEdges.inl"
    "ZenGin/Gothic_UserAPI/zTPMVertexUpdate.inl"
    "ZenGin/Gothic_UserAPI/zTPMWedge.inl"
    "ZenGin/Gothic_UserAPI/zTPolyNode.inl"
    "ZenGin/Gothic_UserAPI/zTProgressRange.inl"
)
source_group("Engine SDK\\User API\\zEngine\\P" FILES ${Engine_SDK__User_API__zEngine__P})

set(Engine_SDK__User_API__zEngine__Q
    "ZenGin/Gothic_UserAPI/zCQuadMark.inl"
    "ZenGin/Gothic_UserAPI/zCQuadMark_zTEffectParams.inl"
    "ZenGin/Gothic_UserAPI/zCQuadMark_zTQuadMarkAniState.inl"
    "ZenGin/Gothic_UserAPI/zCQuadMark_zTQuadMarkPoly.inl"
    "ZenGin/Gothic_UserAPI/zCQuadMark_zTQuadMarkVert.inl"
    "ZenGin/Gothic_UserAPI/zCQuat.inl"
)
source_group("Engine SDK\\User API\\zEngine\\Q" FILES ${Engine_SDK__User_API__zEngine__Q})

set(Engine_SDK__User_API__zEngine__R
    "ZenGin/Gothic_UserAPI/zCRayCache.inl"
    "ZenGin/Gothic_UserAPI/zCRayCacheElement.inl"
    "ZenGin/Gothic_UserAPI/zCRayTurbo.inl"
    "ZenGin/Gothic_UserAPI/zCRayTurboAdmin.inl"
    "ZenGin/Gothic_UserAPI/zCRenderer.inl"
    "ZenGin/Gothic_UserAPI/zCRenderer_zTMaterial.inl"
    "ZenGin/Gothic_UserAPI/zCRenderLight.inl"
    "ZenGin/Gothic_UserAPI/zCRenderLightContainer.inl"
    "ZenGin/Gothic_UserAPI/zCRenderLightContainer_zTLightScore.inl"
    "ZenGin/Gothic_UserAPI/zCRenderManager.inl"
    "ZenGin/Gothic_UserAPI/zCResource.inl"
    "ZenGin/Gothic_UserAPI/zCResourceManager.inl"
    "ZenGin/Gothic_UserAPI/zCResourceManager_zCClassCache.inl"
    "ZenGin/Gothic_UserAPI/zCRigidBody.inl"
    "ZenGin/Gothic_UserAPI/zCRnd_D3D.inl"
    "ZenGin/Gothic_UserAPI/zCRnd_D3D_zD3D_Caps.inl"
    "ZenGin/Gothic_UserAPI/zCRnd_D3D_zD3D_Status.inl"
    "ZenGin/Gothic_UserAPI/zCRnd_Glide.inl"
    "ZenGin/Gothic_UserAPI/zCRndAlphaSortObject.inl"
    "ZenGin/Gothic_UserAPI/zCRoute.inl"
    "ZenGin/Gothic_UserAPI/zRandom_Gauss.inl"
    "ZenGin/Gothic_UserAPI/zSRndGlide_AlphaPoly.inl"
    "ZenGin/Gothic_UserAPI/zTRandAni.inl"
    "ZenGin/Gothic_UserAPI/zTRenderContext.inl"
    "ZenGin/Gothic_UserAPI/zTRnd_DeviceInfo.inl"
    "ZenGin/Gothic_UserAPI/zTRnd_Stats.inl"
    "ZenGin/Gothic_UserAPI/zTRnd_TexFormatInfo.inl"
    "ZenGin/Gothic_UserAPI/zTRnd_VidModeInfo.inl"
    "ZenGin/Gothic_UserAPI/zTRndSimpleVertex.inl"
    "ZenGin/Gothic_UserAPI/zTRndSurfaceDesc.inl"
)
source_group("Engine SDK\\User API\\zEngine\\R" FILES ${Engine_SDK__User_API__zEngine__R})

set(Engine_SDK__User_API__zEngine__S
    "ZenGin/Gothic_UserAPI/zCScanDir.inl"
    "ZenGin/Gothic_UserAPI/zCScanDirFileHandler.inl"
    "ZenGin/Gothic_UserAPI/zCSession.inl"
    "ZenGin/Gothic_UserAPI/zCShader.inl"
    "ZenGin/Gothic_UserAPI/zCShaderStage.inl"
    "ZenGin/Gothic_UserAPI/zCSkyControler.inl"
    "ZenGin/Gothic_UserAPI/zCSkyControler_Indoor.inl"
    "ZenGin/Gothic_UserAPI/zCSkyControler_Mid.inl"
    "ZenGin/Gothic_UserAPI/zCSkyControler_Outdoor.inl"
    "ZenGin/Gothic_UserAPI/zCSkyControler_Outdoor_zTRainFX.inl"
    "ZenGin/Gothic_UserAPI/zCSkyLayer.inl"
    "ZenGin/Gothic_UserAPI/zCSkyLayerData.inl"
    "ZenGin/Gothic_UserAPI/zCSkyPlanet.inl"
    "ZenGin/Gothic_UserAPI/zCSkyState.inl"
    "ZenGin/Gothic_UserAPI/zCSndChannel.inl"
    "ZenGin/Gothic_UserAPI/zCSndFrame.inl"
    "ZenGin/Gothic_UserAPI/zCSndFX_MSS.inl"
    "ZenGin/Gothic_UserAPI/zCSndSys_MSS.inl"
    "ZenGin/Gothic_UserAPI/zCSoundFX.inl"
    "ZenGin/Gothic_UserAPI/zCSoundFXDummy.inl"
    "ZenGin/Gothic_UserAPI/zCSoundManager.inl"
    "ZenGin/Gothic_UserAPI/zCSoundManager_zCMediumType.inl"
    "ZenGin/Gothic_UserAPI/zCSoundManager_zCSoundEventData.inl"
    "ZenGin/Gothic_UserAPI/zCSoundManager_zCTableEntry.inl"
    "ZenGin/Gothic_UserAPI/zCSoundManager_zTScriptSoundData.inl"
    "ZenGin/Gothic_UserAPI/zCSoundSystem.inl"
    "ZenGin/Gothic_UserAPI/zCSoundSystem_zTSound3DParams.inl"
    "ZenGin/Gothic_UserAPI/zCSoundSystemDummy.inl"
    "ZenGin/Gothic_UserAPI/zCSparseArray.inl"
    "ZenGin/Gothic_UserAPI/zCSparseArrayBase.inl"
    "ZenGin/Gothic_UserAPI/zCSparseArrayIterator.inl"
    "ZenGin/Gothic_UserAPI/zCSparseArrayManDat.inl"
    "ZenGin/Gothic_UserAPI/zCSphereCoordsCache.inl"
    "ZenGin/Gothic_UserAPI/zCSphereCoordsCache_zSAziSphereCoord.inl"
    "ZenGin/Gothic_UserAPI/zCSphereCoordsCache_zSElevSphereCoord.inl"
    "ZenGin/Gothic_UserAPI/zCSphereCoordsCache_zSRangeSphereCoord.inl"
    "ZenGin/Gothic_UserAPI/zCSurfaceCache_D3D.inl"
    "ZenGin/Gothic_UserAPI/zCSurfaceCache_D3D_zCSlotIndex.inl"
    "ZenGin/Gothic_UserAPI/zCSurfaceCache_D3D_zD3D_Entry.inl"
    "ZenGin/Gothic_UserAPI/zCSurfaceCache_D3D_zD3D_SlotHeader.inl"
    "ZenGin/Gothic_UserAPI/zCSyncObject.inl"
    "ZenGin/Gothic_UserAPI/zCSystemInfo.inl"
)
source_group("Engine SDK\\User API\\zEngine\\S" FILES ${Engine_SDK__User_API__zEngine__S})

set(Engine_SDK__User_API__zEngine__T
    "ZenGin/Gothic_UserAPI/zCTex_D3D.inl"
    "ZenGin/Gothic_UserAPI/zCTex_RndGlide.inl"
    "ZenGin/Gothic_UserAPI/zCTexAniCtrl.inl"
    "ZenGin/Gothic_UserAPI/zCTexConGeneric.inl"
    "ZenGin/Gothic_UserAPI/zCTexture.inl"
    "ZenGin/Gothic_UserAPI/zCTextureConvert.inl"
    "ZenGin/Gothic_UserAPI/zCTextureExchange.inl"
    "ZenGin/Gothic_UserAPI/zCTextureFileFormat.inl"
    "ZenGin/Gothic_UserAPI/zCTextureFileFormatInternal.inl"
    "ZenGin/Gothic_UserAPI/zCTextureFileFormatTGA.inl"
    "ZenGin/Gothic_UserAPI/zCTextureFileFormatTGA_zTTgaHeader.inl"
    "ZenGin/Gothic_UserAPI/zCTextureFileHandler.inl"
    "ZenGin/Gothic_UserAPI/zCTextureInfo.inl"
    "ZenGin/Gothic_UserAPI/zCThread.inl"
    "ZenGin/Gothic_UserAPI/zCTimer.inl"
    "ZenGin/Gothic_UserAPI/zCTouchAnimate.inl"
    "ZenGin/Gothic_UserAPI/zCTouchAnimateSound.inl"
    "ZenGin/Gothic_UserAPI/zCTouchDamage.inl"
    "ZenGin/Gothic_UserAPI/zCTransfer.inl"
    "ZenGin/Gothic_UserAPI/zCTransferConstr.inl"
    "ZenGin/Gothic_UserAPI/zCTree.inl"
    "ZenGin/Gothic_UserAPI/zCTrigger.inl"
    "ZenGin/Gothic_UserAPI/zCTriggerBase.inl"
    "ZenGin/Gothic_UserAPI/zCTriggerList.inl"
    "ZenGin/Gothic_UserAPI/zCTriggerTeleport.inl"
    "ZenGin/Gothic_UserAPI/zCTriggerUntouch.inl"
    "ZenGin/Gothic_UserAPI/zCTriggerWorldStart.inl"
    "ZenGin/Gothic_UserAPI/zTTexPalette.inl"
    "ZenGin/Gothic_UserAPI/zTTraceRayReport.inl"
)
source_group("Engine SDK\\User API\\zEngine\\T" FILES ${Engine_SDK__User_API__zEngine__T})

set(Engine_SDK__User_API__zEngine__U
    "ZenGin/Gothic_UserAPI/zCUnderwaterPFX.inl"
)
source_group("Engine SDK\\User API\\zEngine\\U" FILES ${Engine_SDK__User_API__zEngine__U})

set(Engine_SDK__User_API__zEngine__V
    "ZenGin/Gothic_UserAPI/zCValueSmooth.inl"
    "ZenGin/Gothic_UserAPI/zCVEC3Smooth.inl"
    "ZenGin/Gothic_UserAPI/zCVertex.inl"
    "ZenGin/Gothic_UserAPI/zCVertexBuffer.inl"
    "ZenGin/Gothic_UserAPI/zCVertexBuffer_D3D.inl"
    "ZenGin/Gothic_UserAPI/zCVertexBuffer_zTCallbackData.inl"
    "ZenGin/Gothic_UserAPI/zCVertexBufferDyn.inl"
    "ZenGin/Gothic_UserAPI/zCVertexBufferDyn2.inl"
    "ZenGin/Gothic_UserAPI/zCVertexBufferManager.inl"
    "ZenGin/Gothic_UserAPI/zCVertexFeaturePool.inl"
    "ZenGin/Gothic_UserAPI/zCVertexPool.inl"
    "ZenGin/Gothic_UserAPI/zCVertexTransform.inl"
    "ZenGin/Gothic_UserAPI/zCVertFeature.inl"
    "ZenGin/Gothic_UserAPI/zCVideoPlayer.inl"
    "ZenGin/Gothic_UserAPI/zCView.inl"
    "ZenGin/Gothic_UserAPI/zCViewBase.inl"
    "ZenGin/Gothic_UserAPI/zCViewDialog.inl"
    "ZenGin/Gothic_UserAPI/zCViewDialogChoice.inl"
    "ZenGin/Gothic_UserAPI/zCViewDraw.inl"
    "ZenGin/Gothic_UserAPI/zCViewFont.inl"
    "ZenGin/Gothic_UserAPI/zCViewFX.inl"
    "ZenGin/Gothic_UserAPI/zCViewObject.inl"
    "ZenGin/Gothic_UserAPI/zCViewPrint.inl"
    "ZenGin/Gothic_UserAPI/zCViewProgressBar.inl"
    "ZenGin/Gothic_UserAPI/zCViewStatusBar.inl"
    "ZenGin/Gothic_UserAPI/zCViewText.inl"
    "ZenGin/Gothic_UserAPI/zCViewText2.inl"
    "ZenGin/Gothic_UserAPI/zCViewWindow.inl"
    "ZenGin/Gothic_UserAPI/zCVisual.inl"
    "ZenGin/Gothic_UserAPI/zCVisualAnimate.inl"
    "ZenGin/Gothic_UserAPI/zCVob.inl"
    "ZenGin/Gothic_UserAPI/zCVob_zTCollisionContext.inl"
    "ZenGin/Gothic_UserAPI/zCVob_zTModelLimbColl.inl"
    "ZenGin/Gothic_UserAPI/zCVobAnimate.inl"
    "ZenGin/Gothic_UserAPI/zCVobCallback.inl"
    "ZenGin/Gothic_UserAPI/zCVobChar.inl"
    "ZenGin/Gothic_UserAPI/zCVobIdentifier.inl"
    "ZenGin/Gothic_UserAPI/zCVobLensFlare.inl"
    "ZenGin/Gothic_UserAPI/zCVobLevelCompo.inl"
    "ZenGin/Gothic_UserAPI/zCVobLight.inl"
    "ZenGin/Gothic_UserAPI/zCVobLightData.inl"
    "ZenGin/Gothic_UserAPI/zCVobLightPreset.inl"
    "ZenGin/Gothic_UserAPI/zCVobScreenFX.inl"
    "ZenGin/Gothic_UserAPI/zCVobScreenFX_zTScreenFXSet.inl"
    "ZenGin/Gothic_UserAPI/zCVobSound.inl"
    "ZenGin/Gothic_UserAPI/zCVobSoundDaytime.inl"
    "ZenGin/Gothic_UserAPI/zCVobSpot.inl"
    "ZenGin/Gothic_UserAPI/zCVobStair.inl"
    "ZenGin/Gothic_UserAPI/zCVobStartpoint.inl"
    "ZenGin/Gothic_UserAPI/zCVobWaypoint.inl"
    "ZenGin/Gothic_UserAPI/zCVolatileMemoryBase.inl"
    "ZenGin/Gothic_UserAPI/zSVisualFXColl.inl"
    "ZenGin/Gothic_UserAPI/zTViewportData.inl"
    "ZenGin/Gothic_UserAPI/zVEC2.inl"
    "ZenGin/Gothic_UserAPI/zVEC3.inl"
    "ZenGin/Gothic_UserAPI/zVEC4.inl"
)
source_group("Engine SDK\\User API\\zEngine\\V" FILES ${Engine_SDK__User_API__zEngine__V})

set(Engine_SDK__User_API__zEngine__W
    "ZenGin/Gothic_UserAPI/zCWaveData.inl"
    "ZenGin/Gothic_UserAPI/zCWay.inl"
    "ZenGin/Gothic_UserAPI/zCWayNet.inl"
    "ZenGin/Gothic_UserAPI/zCWaypoint.inl"
    "ZenGin/Gothic_UserAPI/zCWorld.inl"
    "ZenGin/Gothic_UserAPI/zCWorldInfo.inl"
    "ZenGin/Gothic_UserAPI/zCWorldPerFrameCallback.inl"
)
source_group("Engine SDK\\User API\\zEngine\\W" FILES ${Engine_SDK__User_API__zEngine__W})

set(Engine_SDK__User_API__zEngine__Z
    "ZenGin/Gothic_UserAPI/zCZone.inl"
    "ZenGin/Gothic_UserAPI/zCZoneMusic.inl"
    "ZenGin/Gothic_UserAPI/zCZoneReverb.inl"
    "ZenGin/Gothic_UserAPI/zCZoneReverbDefault.inl"
    "ZenGin/Gothic_UserAPI/zCZoneVobFarPlane.inl"
    "ZenGin/Gothic_UserAPI/zCZoneVobFarPlaneDefault.inl"
    "ZenGin/Gothic_UserAPI/zCZoneZFog.inl"
    "ZenGin/Gothic_UserAPI/zCZoneZFogDefault.inl"
)
source_group("Engine SDK\\User API\\zEngine\\Z" FILES ${Engine_SDK__User_API__zEngine__Z})

set(Plugin__System
    ".build_settings.inl"
    "DllMain.cpp"
    "UnionAfx.h"
)
source_group("Plugin\\System" FILES ${Plugin__System})

set(Plugin__Workspace__Interface
    "Headers.h"
    "Interface.cpp"
    "Sources.h"
)
source_group("Plugin\\Workspace\\Interface" FILES ${Plugin__Workspace__Interface})

set(ALL_FILES
    ${no_group_source_files}
    ${Engine_SDK}
    ${Engine_SDK__1._Gothic_I}
    ${Engine_SDK__1._Gothic_I__Classes__oGame}
    ${Engine_SDK__1._Gothic_I__Classes__zEngine}
    ${Engine_SDK__1._Gothic_I__Overloads}
    ${Engine_SDK__2._Gothic_Sequel}
    ${Engine_SDK__2._Gothic_Sequel__Headers__oGame}
    ${Engine_SDK__2._Gothic_Sequel__Headers__zEngine}
    ${Engine_SDK__2._Gothic_Sequel__Overloads}
    ${Engine_SDK__3._Gothic_II}
    ${Engine_SDK__3._Gothic_II__Headers__oGame}
    ${Engine_SDK__3._Gothic_II__Headers__zEngine}
    ${Engine_SDK__3._Gothic_II__Overloads}
    ${Engine_SDK__4._Gothic_II_NoTR}
    ${Engine_SDK__4._Gothic_II_NoTR__Headers__oGame}
    ${Engine_SDK__4._Gothic_II_NoTR__Headers__zEngine}
    ${Engine_SDK__4._Gothic_II_NoTR__Overloads}
    ${Engine_SDK__User_API__oGame__A}
    ${Engine_SDK__User_API__oGame__B}
    ${Engine_SDK__User_API__oGame__C}
    ${Engine_SDK__User_API__oGame__D}
    ${Engine_SDK__User_API__oGame__E}
    ${Engine_SDK__User_API__oGame__G}
    ${Engine_SDK__User_API__oGame__H}
    ${Engine_SDK__User_API__oGame__I}
    ${Engine_SDK__User_API__oGame__L}
    ${Engine_SDK__User_API__oGame__M}
    ${Engine_SDK__User_API__oGame__N}
    ${Engine_SDK__User_API__oGame__O}
    ${Engine_SDK__User_API__oGame__P}
    ${Engine_SDK__User_API__oGame__R}
    ${Engine_SDK__User_API__oGame__S}
    ${Engine_SDK__User_API__oGame__T}
    ${Engine_SDK__User_API__oGame__V}
    ${Engine_SDK__User_API__oGame__W}
    ${Engine_SDK__User_API__oGame__Z}
    ${Engine_SDK__User_API__zEngine__A}
    ${Engine_SDK__User_API__zEngine__B}
    ${Engine_SDK__User_API__zEngine__C}
    ${Engine_SDK__User_API__zEngine__D}
    ${Engine_SDK__User_API__zEngine__E}
    ${Engine_SDK__User_API__zEngine__F}
    ${Engine_SDK__User_API__zEngine__G}
    ${Engine_SDK__User_API__zEngine__H}
    ${Engine_SDK__User_API__zEngine__I}
    ${Engine_SDK__User_API__zEngine__K}
    ${Engine_SDK__User_API__zEngine__L}
    ${Engine_SDK__User_API__zEngine__M}
    ${Engine_SDK__User_API__zEngine__N}
    ${Engine_SDK__User_API__zEngine__O}
    ${Engine_SDK__User_API__zEngine__P}
    ${Engine_SDK__User_API__zEngine__Q}
    ${Engine_SDK__User_API__zEngine__R}
    ${Engine_SDK__User_API__zEngine__S}
    ${Engine_SDK__User_API__zEngine__T}
    ${Engine_SDK__User_API__zEngine__U}
    ${Engine_SDK__User_API__zEngine__V}
    ${Engine_SDK__User_API__zEngine__W}
    ${Engine_SDK__User_API__zEngine__Z}
    ${Plugin__System}
    ${Plugin__Workspace__Interface}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} SHARED ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE GothicCoop)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
)
################################################################################
# Target name
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
    TARGET_NAME_G2AMDRELEASE  "${PROJECT_NAME}"
    TARGET_NAME_G2AMTRELEASE  "${PROJECT_NAME}"
)
################################################################################
# Output directory
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_DIRECTORY_G1DEBUG       "${CMAKE_SOURCE_DIR}/Bin/"
    OUTPUT_DIRECTORY_G1MDRELEASE   "${CMAKE_SOURCE_DIR}/Bin/"
    OUTPUT_DIRECTORY_G1MTRELEASE   "${CMAKE_SOURCE_DIR}/Bin/"
    OUTPUT_DIRECTORY_G1ADEBUG      "${CMAKE_SOURCE_DIR}/Bin/"
    OUTPUT_DIRECTORY_G1AMDRELEASE  "${CMAKE_SOURCE_DIR}/Bin/"
    OUTPUT_DIRECTORY_G1AMTRELEASE  "${CMAKE_SOURCE_DIR}/Bin/"
    OUTPUT_DIRECTORY_G2DEBUG       "${CMAKE_SOURCE_DIR}/Bin/"
    OUTPUT_DIRECTORY_G2MDRELEASE   "${CMAKE_SOURCE_DIR}/Bin/"
    OUTPUT_DIRECTORY_G2MTRELEASE   "${CMAKE_SOURCE_DIR}/Bin/"
    OUTPUT_DIRECTORY_G2ADEBUG      "${CMAKE_SOURCE_DIR}/Bin/"
    OUTPUT_DIRECTORY_G2AMDRELEASE  "${CMAKE_SOURCE_DIR}/Bin/"
    OUTPUT_DIRECTORY_G2AMTRELEASE  "${CMAKE_SOURCE_DIR}/Bin/"
    OUTPUT_DIRECTORY_MPX2DEBUG     "${CMAKE_SOURCE_DIR}/Bin/"
    OUTPUT_DIRECTORY_MPX2MDRELEASE "${CMAKE_SOURCE_DIR}/Bin/"
    OUTPUT_DIRECTORY_MPX2MTRELEASE "${CMAKE_SOURCE_DIR}/Bin/"
    OUTPUT_DIRECTORY_MPX4DEBUG     "${CMAKE_SOURCE_DIR}/Bin/"
    OUTPUT_DIRECTORY_MPX4MDRELEASE "${CMAKE_SOURCE_DIR}/Bin/"
    OUTPUT_DIRECTORY_MPX4MTRELEASE "${CMAKE_SOURCE_DIR}/Bin/"
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    PDB_OUTPUT_DIRECTORY               "${CMAKE_SOURCE_DIR}/PDB"
)
set_target_properties(${PROJECT_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_G1MDRELEASE   "TRUE"
    INTERPROCEDURAL_OPTIMIZATION_G1MTRELEASE   "TRUE"
    INTERPROCEDURAL_OPTIMIZATION_G1AMDRELEASE  "TRUE"
    INTERPROCEDURAL_OPTIMIZATION_G1AMTRELEASE  "TRUE"
    INTERPROCEDURAL_OPTIMIZATION_G2MDRELEASE   "TRUE"
    INTERPROCEDURAL_OPTIMIZATION_G2MTRELEASE   "TRUE"
    INTERPROCEDURAL_OPTIMIZATION_G2AMDRELEASE  "TRUE"
    INTERPROCEDURAL_OPTIMIZATION_G2AMTRELEASE  "TRUE"
    INTERPROCEDURAL_OPTIMIZATION_MPX2MDRELEASE "TRUE"
    INTERPROCEDURAL_OPTIMIZATION_MPX2MTRELEASE "TRUE"
    INTERPROCEDURAL_OPTIMIZATION_MPX4MDRELEASE "TRUE"
    INTERPROCEDURAL_OPTIMIZATION_MPX4MTRELEASE "TRUE"
)
################################################################################
# MSVC runtime library
################################################################################
get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)
string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
    $<$<CONFIG:G1Debug>:
        MultiThreadedDebugDLL
    >
    $<$<CONFIG:G1MDRelease>:
        MultiThreadedDLL
    >
    $<$<CONFIG:G1MTRelease>:
        MultiThreaded
    >
    $<$<CONFIG:G1ADebug>:
        MultiThreadedDLL
    >
    $<$<CONFIG:G1AMDRelease>:
        MultiThreadedDLL
    >
    $<$<CONFIG:G1AMTRelease>:
        MultiThreaded
    >
    $<$<CONFIG:G2Debug>:
        MultiThreadedDLL
    >
    $<$<CONFIG:G2MDRelease>:
        MultiThreadedDLL
    >
    $<$<CONFIG:G2MTRelease>:
        MultiThreaded
    >
    $<$<CONFIG:G2ADebug>:
        MultiThreadedDebugDLL
    >
    $<$<CONFIG:G2AMDRelease>:
        MultiThreadedDLL
    >
    $<$<CONFIG:G2AMTRelease>:
        MultiThreaded
    >
    $<$<CONFIG:MPx2Debug>:
        MultiThreadedDLL
    >
    $<$<CONFIG:MPx2MDRelease>:
        MultiThreadedDLL
    >
    $<$<CONFIG:MPx2MTRelease>:
        MultiThreaded
    >
    $<$<CONFIG:MPx4Debug>:
        MultiThreadedDLL
    >
    $<$<CONFIG:MPx4MDRelease>:
        MultiThreadedDLL
    >
    $<$<CONFIG:MPx4MTRelease>:
        MultiThreaded
    >
    $<$<NOT:$<OR:$<CONFIG:G1Debug>,$<CONFIG:G1MDRelease>,$<CONFIG:G1MTRelease>,$<CONFIG:G1ADebug>,$<CONFIG:G1AMDRelease>,$<CONFIG:G1AMTRelease>,$<CONFIG:G2Debug>,$<CONFIG:G2MDRelease>,$<CONFIG:G2MTRelease>,$<CONFIG:G2ADebug>,$<CONFIG:G2AMDRelease>,$<CONFIG:G2AMTRelease>,$<CONFIG:MPx2Debug>,$<CONFIG:MPx2MDRelease>,$<CONFIG:MPx2MTRelease>,$<CONFIG:MPx4Debug>,$<CONFIG:MPx4MDRelease>,$<CONFIG:MPx4MTRelease>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
)
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

################################################################################
# Include directories
################################################################################
target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/enet-1.3.17/include;"
    "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/nlohmann-json/include/single_include"
)

################################################################################
# Compile definitions
################################################################################
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:G1Debug>:"
        "_DEBUG;"
        "__G1"
    ">"
    "$<$<CONFIG:G1MDRelease>:"
        "NDEBUG;"
        "__G1"
    ">"
    "$<$<CONFIG:G1MTRelease>:"
        "NDEBUG;"
        "__G1"
    ">"
    "$<$<CONFIG:G1ADebug>:"
        "_DEBUG;"
        "__G1A"
    ">"
    "$<$<CONFIG:G1AMDRelease>:"
        "NDEBUG;"
        "__G1A"
    ">"
    "$<$<CONFIG:G1AMTRelease>:"
        "NDEBUG;"
        "__G1A"
    ">"
    "$<$<CONFIG:G2Debug>:"
        "_DEBUG;"
        "__G2"
    ">"
    "$<$<CONFIG:G2MDRelease>:"
        "NDEBUG;"
        "__G2"
    ">"
    "$<$<CONFIG:G2MTRelease>:"
        "NDEBUG;"
        "__G2"
    ">"
    "$<$<CONFIG:G2ADebug>:"
        "_DEBUG;"
        "__G2A"
    ">"
    "$<$<CONFIG:G2AMDRelease>:"
        "NDEBUG;"
        "__G2A"
    ">"
    "$<$<CONFIG:G2AMTRelease>:"
        "NDEBUG;"
        "__G2A"
    ">"
    "$<$<CONFIG:MPx2Debug>:"
        "_DEBUG;"
        "__G1;"
        "__G2A"
    ">"
    "$<$<CONFIG:MPx2MDRelease>:"
        "NDEBUG;"
        "__G1;"
        "__G2A"
    ">"
    "$<$<CONFIG:MPx2MTRelease>:"
        "NDEBUG;"
        "__G1;"
        "__G2A"
    ">"
    "$<$<CONFIG:MPx4Debug>:"
        "_DEBUG;"
        "__G1;"
        "__G1A;"
        "__G2;"
        "__G2A"
    ">"
    "$<$<CONFIG:MPx4MDRelease>:"
        "NDEBUG;"
        "__G1;"
        "__G1A;"
        "__G2;"
        "__G2A"
    ">"
    "$<$<CONFIG:MPx4MTRelease>:"
        "NDEBUG;"
        "__G1;"
        "__G1A;"
        "__G2;"
        "__G2A"
    ">"
    "WIN32;"
    "_CRT_SECURE_NO_WARNINGS;"
    "_WINSOCK_DEPRECATED_NO_WARNINGS;"
    "_WINDOWS;"
    "_USRDLL;"
    "GOTHICPLUGIN_EXPORTS;"
    "_MBCS"
)

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:G1Debug>:
            /Od;
            /Gh;
            /EHa
        >
        $<$<CONFIG:G1MDRelease>:
            /GL-;
            /O2;
            /Oi;
            /Gy;
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        >
        $<$<CONFIG:G1MTRelease>:
            /GL-;
            /Od;
            /Oi;
            /Gy;
            /Gh;
            /EHa
        >
        $<$<CONFIG:G1ADebug>:
            /Od;
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        >
        $<$<CONFIG:G1AMDRelease>:
            /O2;
            /Oi;
            /Gy;
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        >
        $<$<CONFIG:G1AMTRelease>:
            /O2;
            /Oi;
            /Gy;
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        >
        $<$<CONFIG:G2Debug>:
            /Od
        >
        $<$<CONFIG:G2MDRelease>:
            /O2;
            /Oi;
            /Gy;
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        >
        $<$<CONFIG:G2MTRelease>:
            /O2;
            /Oi;
            /Gy;
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        >
        $<$<CONFIG:G2ADebug>:
            /Od;
            /Gh;
            /EHa
        >
        $<$<CONFIG:G2AMDRelease>:
            /O2;
            /Oi;
            /Gy;
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        >
        $<$<CONFIG:G2AMTRelease>:
            /GL-;
            /Od;
            /Oi;
            /Gy;
            /Gh;
            /EHa
        >
        $<$<CONFIG:MPx2Debug>:
            /Od;
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        >
        $<$<CONFIG:MPx2MDRelease>:
            /O2;
            /Oi;
            /Gy;
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        >
        $<$<CONFIG:MPx2MTRelease>:
            /O2;
            /Oi;
            /Gy;
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        >
        $<$<CONFIG:MPx4Debug>:
            /Od;
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        >
        $<$<CONFIG:MPx4MDRelease>:
            /O2;
            /Oi;
            /Gy;
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        >
        $<$<CONFIG:MPx4MTRelease>:
            /O2;
            /Oi;
            /Gy;
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        >
        /MP;
        /sdl;
        /W3;
        ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
        /Y-
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:G1Debug>:
            /INCREMENTAL
        >
        $<$<CONFIG:G1MDRelease>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        $<$<CONFIG:G1MTRelease>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        $<$<CONFIG:G1ADebug>:
            /INCREMENTAL
        >
        $<$<CONFIG:G1AMDRelease>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        $<$<CONFIG:G1AMTRelease>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        $<$<CONFIG:G2Debug>:
            /INCREMENTAL
        >
        $<$<CONFIG:G2MDRelease>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        $<$<CONFIG:G2MTRelease>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        $<$<CONFIG:G2ADebug>:
            /INCREMENTAL
        >
        $<$<CONFIG:G2AMDRelease>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        $<$<CONFIG:G2AMTRelease>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        $<$<CONFIG:MPx2Debug>:
            /INCREMENTAL
        >
        $<$<CONFIG:MPx2MDRelease>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        $<$<CONFIG:MPx2MTRelease>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        $<$<CONFIG:MPx4Debug>:
            /INCREMENTAL
        >
        $<$<CONFIG:MPx4MDRelease>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        $<$<CONFIG:MPx4MTRelease>:
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        /DEBUG;
        /SAFESEH:NO;
        /SUBSYSTEM:WINDOWS
    )
endif()

################################################################################
# Pre build events
################################################################################
add_custom_command_if(
    TARGET ${PROJECT_NAME}
    PRE_BUILD
    COMMANDS
    COMMAND       $<CONFIG:G1Debug> $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\SDK\\Union\\v1.0l\\Build\\beforebuild.exe $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>
    COMMAND   $<CONFIG:G1MDRelease> $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\SDK\\Union\\v1.0l\\Build\\beforebuild.exe $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>
    COMMAND   $<CONFIG:G1MTRelease> $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\SDK\\Union\\v1.0l\\Build\\beforebuild.exe $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>
    COMMAND      $<CONFIG:G1ADebug> $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\SDK\\Union\\v1.0l\\Build\\beforebuild.exe $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>
    COMMAND  $<CONFIG:G1AMDRelease> $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\SDK\\Union\\v1.0l\\Build\\beforebuild.exe $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>
    COMMAND  $<CONFIG:G1AMTRelease> $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\SDK\\Union\\v1.0l\\Build\\beforebuild.exe $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>
    COMMAND       $<CONFIG:G2Debug> $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\SDK\\Union\\v1.0l\\Build\\beforebuild.exe $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>
    COMMAND   $<CONFIG:G2MDRelease> $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\SDK\\Union\\v1.0l\\Build\\beforebuild.exe $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>
    COMMAND   $<CONFIG:G2MTRelease> $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\SDK\\Union\\v1.0l\\Build\\beforebuild.exe $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>
    COMMAND      $<CONFIG:G2ADebug> $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\SDK\\Union\\v1.0l\\Build\\beforebuild.exe $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>
    COMMAND  $<CONFIG:G2AMDRelease> $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\SDK\\Union\\v1.0l\\Build\\beforebuild.exe $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>
    COMMAND  $<CONFIG:G2AMTRelease> $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\SDK\\Union\\v1.0l\\Build\\beforebuild.exe $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>
    COMMAND     $<CONFIG:MPx2Debug> $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\SDK\\Union\\v1.0l\\Build\\beforebuild.exe $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>
    COMMAND $<CONFIG:MPx2MDRelease> $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\SDK\\Union\\v1.0l\\Build\\beforebuild.exe $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>
    COMMAND $<CONFIG:MPx2MTRelease> $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\SDK\\Union\\v1.0l\\Build\\beforebuild.exe $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>
    COMMAND     $<CONFIG:MPx4Debug> $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\SDK\\Union\\v1.0l\\Build\\beforebuild.exe $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>
    COMMAND $<CONFIG:MPx4MDRelease> $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\SDK\\Union\\v1.0l\\Build\\beforebuild.exe $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>
    COMMAND $<CONFIG:MPx4MTRelease> $<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/>\\SDK\\Union\\v1.0l\\Build\\beforebuild.exe $<SHELL_PATH:${CMAKE_SOURCE_DIR}/>
)

################################################################################
# Dependencies
################################################################################
set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "enet;"
    "ws2_32;"
    "winmm;"
    "DbgHelp;"
)
target_link_libraries(${PROJECT_NAME} PUBLIC "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

target_link_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/./Dependencies/enet-1.3.17"
	"${CMAKE_CURRENT_SOURCE_DIR}/./SDK/Union/v1.0l/Lib"
)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/SDK/Union/v1.0l/Include
	${CMAKE_CURRENT_SOURCE_DIR}/ZenGin/Gothic_UserAPI
)
