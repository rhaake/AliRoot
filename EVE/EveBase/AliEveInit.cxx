//
//  AliEveInit.cpp
//  xAliRoot
//
//  Created by Jeremi Niedziela on 01/06/15.
//
//

#include "AliEveInit.h"

#include <TString.h>
#include <TGrid.h>
#include <TSystem.h>
#include <TROOT.h>
#include <TInterpreter.h>
#include <TMath.h>
#include <TGListTree.h>
#include <TEveVSDStructs.h>
#include <TEveManager.h>
#include <TEveTrackPropagator.h>
#include <TEnv.h>
#include <TEveWindowManager.h>
#include <TGTab.h>
#include <TTimeStamp.h>
#include <TPRegexp.h>
#include <TFolder.h>
#include <TSystemDirectory.h>
#include <TGButton.h>
#include <TGFileBrowser.h>
#include <TEveMacro.h>

#include <AliESDtrackCuts.h>
#include <AliESDEvent.h>
#include <AliESDfriend.h>
#include <AliESDtrack.h>
#include <AliESDfriendTrack.h>
#include <AliExternalTrackParam.h>

#include <AliEveTrack.h>
#include <AliEveTrackCounter.h>
#include <AliEveMagField.h>
#include <AliEveEventManagerEditor.h>
#include <AliEveMultiView.h>
#include <AliEveMacroExecutor.h>
#include <AliEveMacro.h>
#include <AliEveMacroExecutorWindow.h>
#include <AliEveEventSelectorWindow.h>
#include <AliEveTrackFitter.h>
#include <AliEveGeomGentle.h>
#include <AliEveDataSourceOffline.h>
#include <AliEveDataSourceHLTZMQ.h>
#include <AliEveEventManager.h>

#include <AliCDBManager.h>

#include <iostream>

using namespace std;

AliEveInit::AliEveInit(const TString& path ,AliEveEventManager::EDataSource defaultDataSource,bool storageManager) :
    fPath(path)
{
    //==============================================================================
    // Reading preferences from config file
    //==============================================================================
    
    TEnv settings;
    GetConfig(&settings);
    
    fShowHLTESDtree       = settings.GetValue("HLTESDtree.show", false);      // show HLT ESD tree
    bool showMuon         = settings.GetValue("MUON.show", true);             // show MUON's geom
    Color_t colorTRD      = settings.GetValue("TRD.geom.color", kGray);       // color of TRD modules
    Color_t colorMUON     = settings.GetValue("MUON.geom.color",kGray);       // color of MUON modules

    Width_t width         = settings.GetValue("tracks.width",2);              // width of all ESD tracks
    bool dashNoRefit      = settings.GetValue("tracks.noRefit.dash",true);    // dash no-refit tracks
    bool drawNoRefit      = settings.GetValue("tracks.noRefit.show",true);    // show no-refit tracks
    bool tracksByType     = settings.GetValue("tracks.byType.show",true);     // colorize tracks by PID
    bool tracksByCategory = settings.GetValue("tracks.byCategory.show",false);// colorize tracks by reconstruction quality

    bool autoloadEvents   = settings.GetValue("events.autoload.set",false);   // set autoload by default
    
    bool drawClusters     = settings.GetValue("clusters.show",false);          // show clusters
    bool drawKinks        = settings.GetValue("kinks.show",false);             // show kinks
    bool drawV0s          = settings.GetValue("V0s.show",false);               // show V0s
    bool drawCascades     = settings.GetValue("cascades.show",false);          // show cascades
    bool drawRawData      = settings.GetValue("rawData.show",false);           // show raw data
    bool drawPrimaryVertex= settings.GetValue("primary.vertex.show",false);    // show primary vertex
    bool drawHits         = settings.GetValue("hits.show",false);              // show hits
    bool drawDigits       = settings.GetValue("digits.show",false);              // show digits
    bool saveViews        = settings.GetValue("ALICE_LIVE.send",false);       // send pictures to ALICE LIVE
    
    TString ocdbStorage   = settings.GetValue("OCDB.default.path","local://$ALICE_ROOT/../src/OCDB");// default path to OCDB
    
    
    cout<<"\n\nOCDB path:"<<ocdbStorage<<endl<<endl<<endl;
    
    bool customPreset = false; // should custom colors presets be used
    
    //==============================================================================
    // Event Manager and different data sources
    //==============================================================================

    AliEveEventManager *man = new AliEveEventManager(defaultDataSource);
    
    AliEveEventManager::SetCdbUri(ocdbStorage);
    
    if (gSystem->Getenv("ALICE_ROOT") != 0)
    {
        gInterpreter->AddIncludePath(Form("%s/MUON", gSystem->Getenv("ALICE_ROOT")));
        gInterpreter->AddIncludePath(Form("%s/MUON/mapping", gSystem->Getenv("ALICE_ROOT")));
    }
    
    AliEveDataSourceOffline *dataSourceOffline  = (AliEveDataSourceOffline*)man->GetDataSourceOffline();
    AliEveDataSourceHLTZMQ  *dataSourceHLT      = (AliEveDataSourceHLTZMQ*) man->GetDataSourceHLTZMQ();
    
    dataSourceOffline->AddAODfriend("AliAOD.VertexingHF.root");
    
    ImportMacros();
    Init();
    
    TEveUtil::AssertMacro("VizDB_scan.C");
    
    TEveBrowser *browser = gEve->GetBrowser();
    browser->ShowCloseTab(kFALSE);
    
    //==============================================================================
    // Geometry, scenes, projections and viewers
    //==============================================================================
    
    AliEveMultiView *mv = new AliEveMultiView(false);
    AliEveGeomGentle *geomGentle = new AliEveGeomGentle();

    mv->SetDepth(-10);

    mv->InitGeomGentle(geomGentle->GetGeomGentle(),
                              geomGentle->GetGeomGentleRphi(),
                              geomGentle->GetGeomGentleRhoz(),
                              0/*geomGentle->GetGeomGentleRhoz()*/);
    
    mv->InitGeomGentleTrd(geomGentle->GetGeomGentleTRD(colorTRD));
    mv->InitGeomGentleMuon(geomGentle->GetGeomGentleMUON(true,colorMUON), kFALSE, kTRUE, kFALSE);

    mv->SetDepth(0);
    
    //==============================================================================
    // Registration of per-event macros
    //==============================================================================
    
    AliEveMacroExecutor *exec = man->GetExecutor();
    
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "SIM Track",   "kine_tracks.C", "kine_tracks", "", kFALSE));
    
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "SIM Hits ITS", "its_hits.C",    "its_hits",    "", drawHits));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "SIM Hits TPC", "tpc_hits.C",    "tpc_hits",    "", drawHits));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "SIM Hits T0",  "t0_hits.C",     "t0_hits",     "", drawHits));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "SIM Hits FMD", "fmd_hits.C",    "fmd_hits",    "", drawHits));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "SIM Hits ACORDE", "acorde_hits.C",    "acorde_hits",    "", drawHits));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "SIM Hits EMCAL", "emcal_hits.C",    "emcal_hits",    "", drawHits));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "SIM Hits TOF",  "tof_hits.C",     "tof_hits",     "", drawHits));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "SIM Hits TRD", "trd_hits.C",    "trd_hits",    "", drawHits));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "SIM Hits VZERO", "vzero_hits.C",    "vzero_hits",    "", drawHits));
    
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "DIG ITS",     "its_digits.C",  "its_digits",  "", drawDigits));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "DIG TPC",     "tpc_digits.C",  "tpc_digits",  "", drawDigits));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "DIG TOF",     "tof_digits.C",  "tof_digits",  "", drawDigits));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "DIG HMPID",   "hmpid_digits.C","hmpid_digits","", drawDigits));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "DIG FMD",     "fmd_digits.C",  "fmd_digits",  "", drawDigits));
    
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRawReader, "RAW ITS",     "its_raw.C",     "its_raw",     "", drawRawData));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRawReader, "RAW TPC",     "tpc_raw.C",     "tpc_raw",     "", drawRawData));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRawReader, "RAW TOF",     "tof_raw.C",     "tof_raw",     "", drawRawData));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRawReader, "RAW HMPID",   "hmpid_raw.C",   "hmpid_raw",   "", drawRawData));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRawReader, "RAW T0",      "t0_raw.C",      "t0_raw",      "", drawRawData));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRawReader, "RAW FMD",     "fmd_raw.C",     "fmd_raw",     "", drawRawData));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRawReader, "RAW VZERO",   "vzero_raw.C",   "vzero_raw",   "", drawRawData));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRawReader, "RAW ACORDE",  "acorde_raw.C",  "acorde_raw",  "", drawRawData));
    
     exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC PVTX",             "primary_vertex.C", "primary_vertex",             "",                drawPrimaryVertex));
     exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC PVTX Ellipse",     "primary_vertex.C", "primary_vertex_ellipse",     "",                drawPrimaryVertex));
     exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC PVTX Box",         "primary_vertex.C", "primary_vertex_box",         "kFALSE, 3, 3, 3", drawPrimaryVertex));
     exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC PVTX SPD",         "primary_vertex.C", "primary_vertex_spd",         "",                drawPrimaryVertex));
     exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC PVTX Ellipse SPD", "primary_vertex.C", "primary_vertex_ellipse_spd", "",                drawPrimaryVertex));
     exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC PVTX Box SPD",     "primary_vertex.C", "primary_vertex_box_spd",     "kFALSE, 3, 3, 3", drawPrimaryVertex));
     exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC PVTX TPC",         "primary_vertex.C", "primary_vertex_tpc",         "",                drawPrimaryVertex));
     exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC PVTX Ellipse TPC", "primary_vertex.C", "primary_vertex_ellipse_tpc", "",                drawPrimaryVertex));
     exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC PVTX Box TPC",     "primary_vertex.C", "primary_vertex_box_tpc",     "kFALSE, 3, 3, 3", drawPrimaryVertex));
    
    exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC V0",   "esd_V0_points.C",       "esd_V0_points_onfly","",  drawV0s));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC V0",   "esd_V0_points.C",       "esd_V0_points_offline","",drawV0s));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC V0",   "esd_V0.C",              "esd_V0","",               drawV0s));

    exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC CSCD", "esd_cascade_points.C",  "esd_cascade_points","", drawCascades));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC CSCD", "esd_cascade.C",         "esd_cascade","",        drawCascades));
    
    exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC KINK", "esd_kink_points.C",     "esd_kink_points","",    drawKinks));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC KINK", "esd_kink.C",            "esd_kink","",           drawKinks));
    
    // default appearance:
    //  exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC Tracks by category",  "esd_tracks.C", "esd_tracks_by_category",  "", kTRUE));
    
    // preset for cosmics:
    //  exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC Tracks by category",  "esd_tracks.C", "esd_tracks_by_category",  "kGreen,kGreen,kGreen,kGreen,kGreen,kGreen,kGreen,kGreen,kGreen,kFALSE", kTRUE));
    
    
    exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC Tracklets SPD", "esd_spd_tracklets.C", "esd_spd_tracklets", "", kTRUE));
    
    exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC ZDC",      "esd_zdc.C", "esd_zdc", "", kFALSE));
    
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "REC Clusters ITS", "its_clusters.C", "its_clusters","",     drawClusters));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "REC Clusters TPC", "tpc_clusters.C", "tpc_clusters","",     drawClusters));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "REC Clusters TRD", "trd_clusters.C", "trd_clusters","",     drawClusters));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "REC Clusters TOF", "tof_clusters.C", "tof_clusters","",     drawClusters));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "REC Clusters HMPID", "hmpid_clusters.C","hmpid_clusters","",drawClusters));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "REC Clusters PHOS", "phos_clusters.C","phos_clusters","",   drawClusters));
    
    if (showMuon)
    {
        exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "SIM TrackRef MUON", "muon_trackRefs.C", "muon_trackRefs", "kTRUE", kFALSE));
        exec->AddMacro(new AliEveMacro(AliEveMacro::kRawReader, "RAW MUON", "muon_raw.C", "muon_raw", "", drawRawData));
        exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "DIG MUON", "muon_digits.C", "muon_digits", "", drawDigits));
        exec->AddMacro(new AliEveMacro(AliEveMacro::kRunLoader, "REC Clusters MUON", "muon_clusters.C", "muon_clusters", "", drawClusters));
        exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "REC Tracks MUON", "esd_muon_tracks.C", "esd_muon_tracks", "kTRUE,kFALSE", kTRUE));
    }
    exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "ESD AD", "ad_esd.C", "ad_esd", "", kTRUE));
    exec->AddMacro(new AliEveMacro(AliEveMacro::kESD, "ESD EMCal", "emcal_esdclustercells.C", "emcal_esdclustercells", "", kTRUE));
    
    
    //==============================================================================
    // Additional GUI components
    //==============================================================================
    
    // Macro / data selection
    TEveWindowSlot *slot = TEveWindow::CreateWindowInTab(browser->GetTabRight());
    slot->StartEmbedding();
    AliEveMacroExecutorWindow* exewin = new AliEveMacroExecutorWindow(exec);
    slot->StopEmbedding("DataSelection");
    exewin->PopulateMacros();
    
    // Event selection tab
    slot = TEveWindow::CreateWindowInTab(browser->GetTabRight());
    slot->StartEmbedding();
    new AliEveEventSelectorWindow(gClient->GetRoot(), 600, 400, man->GetEventSelector());
    slot->StopEmbedding("Selections");
    
    // QA viewer
    /*
     slot = TEveWindow::CreateWindowInTab(browser->GetTabRight());
     slot->StartEmbedding();
     new AliQAHistViewer(gClient->GetRoot(), 600, 400, kTRUE);
     slot->StopEmbedding("QA histograms");
     */
//    browser->GetTabRight()->SetTab(1);
    browser->StartEmbedding(TRootBrowser::kBottom);
    new AliEveEventManagerWindow(man,storageManager,defaultDataSource);
    browser->StopEmbedding("EventCtrl");
    
    slot = TEveWindow::CreateWindowInTab(browser->GetTabRight());
    TEveWindowTab *store_tab = slot->MakeTab();
    store_tab->SetElementNameTitle("WindowStore",
                                   "Undocked windows whose previous container is not known\n"
                                   "are placed here when the main-frame is closed.");
    gEve->GetWindowManager()->SetDefaultContainer(store_tab);
    
    
    //==============================================================================
    // AliEve objects - global tools
    //==============================================================================
    
    AliEveTrackFitter* fitter = new AliEveTrackFitter();
    gEve->AddToListTree(fitter, 1);
    gEve->AddElement(fitter, gEve->GetEventScene());
    
    AliEveTrackCounter* g_trkcnt = new AliEveTrackCounter("Primary Counter");
    gEve->AddToListTree(g_trkcnt, kFALSE);
    
    
    //==============================================================================
    // Final stuff
    //==============================================================================
    
    
    // A refresh to show proper window.
//    gEve->GetViewers()->SwitchColorSet();

    browser->MoveResize(0, 0, gClient->GetDisplayWidth(),gClient->GetDisplayHeight() - 32);
    gEve->Redraw3D(true);
    gSystem->ProcessEvents();
    
    man->GotoEvent(0);
    
    gEve->EditElement(g_trkcnt);
    gEve->Redraw3D();
    
    // move and rotate sub-views
    browser->GetTabRight()->SetTab(1);
    TGLViewer *glv1 = mv->Get3DView()->GetGLViewer();
    TGLViewer *glv2 = mv->GetRPhiView()->GetGLViewer();
    TGLViewer *glv3 = mv->GetRhoZView()->GetGLViewer();
    
    glv1->CurrentCamera().RotateRad(-0.4, 0.6);
    glv2->CurrentCamera().Dolly(1, kFALSE, kFALSE);
    glv3->CurrentCamera().Dolly(1, kFALSE, kFALSE);
    
    gEve->FullRedraw3D();
    gSystem->ProcessEvents();
    gEve->Redraw3D(true);
    
    if(customPreset){
        Color_t colors[9] = {kCyan,kCyan,kCyan,kCyan,kCyan,kCyan,kCyan,kCyan,kCyan};
        man->SetESDcolorsByType(colors);
    }
    man->SetESDwidth(width);
    man->SetESDdashNoRefit(dashNoRefit);
    man->SetESDdrawNoRefit(drawNoRefit);
    
    man->SetESDtracksByCategory(tracksByCategory);
    man->SetESDtracksByType(tracksByType);
    
    man->SetSaveViews(saveViews);
    man->SetAutoLoad(autoloadEvents);// set autoload by default
}

void AliEveInit::Init()
{
    const Text_t* esdfile = 0;
    const Text_t* aodfile = 0;
    const Text_t* rawfile = 0;
    
    cout<<"Adding standard macros"<<endl;
    TEveUtil::AssertMacro("VizDB_scan.C");
    gSystem->ProcessEvents();
    
    AliEveDataSourceOffline *dataSource = (AliEveDataSourceOffline*)AliEveEventManager::GetMaster()->GetDataSourceOffline();
    
    dataSource->SetFilesPath(fPath);
    
    if(fShowHLTESDtree){
        dataSource->SetESDFileName(esdfile, AliEveDataSourceOffline::kHLTTree);
    }
    else{
        dataSource->SetESDFileName(esdfile, AliEveDataSourceOffline::kOfflineTree);
    }
    
    dataSource->SetRawFileName(rawfile);
    dataSource->SetAssertElements(0,0,0,0);
    
    // Open event
    if (fPath.BeginsWith("alien:"))
    {
        if (gGrid != 0)
        {
            Info("AliEveInit::Init()", "TGrid already initializied. Skiping checks and initialization.");
        }
        else
        {
            Info("AliEveInit::Init()", "AliEn requested - connecting.");
            if (gSystem->Getenv("GSHELL_ROOT") == 0)
            {
                Error("AliEveInit::Init()", "AliEn environment not initialized. Aborting.");
                gSystem->Exit(1);
            }
            if (TGrid::Connect("alien") == 0)
            {
                Error("AliEveInit::Init()", "TGrid::Connect() failed. Aborting.");
                gSystem->Exit(1);
            }
        }
    }
    cout<<"Opening event -1 from "<<fPath.Data()<<endl;
    gEve->AddEvent(AliEveEventManager::GetMaster());
}

void AliEveInit::ImportMacros()
{
    // Put macros in the list of browsables, add a macro browser to
    // top-level GUI.
    
    TString  hack = gSystem->pwd(); // Problem with TGFileBrowser cding
    
    TString macdir("$(ALICE_ROOT)/EVE/alice-macros");
    gSystem->ExpandPathName(macdir);
    
    TFolder* f = gEve->GetMacroFolder();
    void* dirhandle = gSystem->OpenDirectory(macdir.Data());
    if (dirhandle != 0)
    {
        const char* filename;
        TPMERegexp re("\\.C$");
        TObjArray names;
        while ((filename = gSystem->GetDirEntry(dirhandle)) != 0)
        {
            if (re.Match(filename))
                names.AddLast(new TObjString(filename));
        }
        names.Sort();
        
        for (Int_t ii=0; ii<names.GetEntries(); ++ii)
        {
            TObjString * si = (TObjString*) names.At(ii);
            f->Add(new TEveMacro(Form("%s/%s", macdir.Data(), (si->GetString()).Data())));
        }
    }
    gSystem->FreeDirectory(dirhandle);
    
    gROOT->GetListOfBrowsables()->Add(new TSystemDirectory(macdir.Data(), macdir.Data()));
    
    {
        TEveBrowser   *br = gEve->GetBrowser();
        TGFileBrowser *fb = 0;
        fb = br->GetFileBrowser();
        fb->GotoDir(macdir);
        {
            br->StartEmbedding(0);
            fb = br->MakeFileBrowser();
            fb->BrowseObj(f);
            fb->Show();
            br->StopEmbedding();
            br->SetTabTitle("Macros", 0);
            br->SetTab(0, 0);
        }
    }
    gSystem->cd(hack);
}

void AliEveInit::GetConfig(TEnv *settings)
{
    if(settings->ReadFile(Form("%s/eve_config",gSystem->Getenv("HOME")), kEnvUser) < 0){
        cout<<"Warning - could not find eve_config in home directory! Trying in $ALICE_ROOT/EVE/EveBase/"<<endl;
        if(settings->ReadFile(Form("%s/EVE/EveBase/eve_config",gSystem->Getenv("ALICE_ROOT")), kEnvUser) < 0){
            cout<<"Error - could not find eve_config file!."<<endl;
            exit(0);
        }
    }
}



