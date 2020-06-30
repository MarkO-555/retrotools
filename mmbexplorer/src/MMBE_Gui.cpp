#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
#include <FL/Fl.H>
#include <FL/Fl_Sys_Menu_Bar.H>
#include <FL/Fl_Select_Browser.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include <FL/x.H>
#include "MMBE_Gui.h"
#include "MMBE_Commands.h"
#include "MMBE_Callbacks.h"
#include "AcornDFS.h"

// Icons
#include "../icons/empty.xpm"
#include "../icons/locked.xpm"
#include "../icons/unlocked.xpm"
#include "../logo/mmbexplorer_64x64.xpm"

using namespace std;

CAppWindow::CAppWindow( int _w, int _h, const char* _label ) : 
        Fl_Window( _w, _h, _label ) 
{
}

CAppWindow::CAppWindow( int _x, int _y, int _w, int _h, const char* _label ) : 
    Fl_Window( _x, _y, _w, _h, _label )
{
}

CAppWindow::~CAppWindow()
{

}

int CAppWindow::handle( int _event )
{
    int ret = Fl_Window::handle( _event );
    /*switch ( _event ) 
    {
        case FL_PUSH:               // do 'copy/dnd' when someone clicks on box
            Fl::copy(blabla.c_str(),(int)blabla.length(),0);//Fl::clipboard_image);
            Fl::dnd();
            ret = 1;
            break;
    }*/
    return(ret);
}

void CAppWindow::SetDiskContentWidget( Fl_Select_Browser* _diskContent )
{
    mDiskContent = _diskContent;
}

void CAppWindow::RefreshDiskContent( unsigned char* _data, size_t _dataSize )
{
    if( nullptr == mDiskContent )
    {
        return;
    }

    mDiskContent->clear();    

    if( nullptr != _data )
    {
        stringstream strStream;
        DFSDisk disk;

        DFSRead( _data, _dataSize, disk );

        string diskNameStr = "@fDisk name: " + disk.name;
        mDiskContent->add( diskNameStr.c_str() );
        string bootOptionStr = "@fBoot option: ";
        bootOptionStr += BootOptionToString( disk.bootOption );
        mDiskContent->add( bootOptionStr.c_str() );
        mDiskContent->add( "@f-+-+----------+------+-----+-----");
        mDiskContent->add( "@fL|D|File name |Size  |Load |Exec ");
        mDiskContent->add( "@f-+-+----------+------+-----+-----");
        
        size_t sectorsUsed = 2; // Filesystem sectors

        for( auto dfsFile : disk.files )
        {
            string fileStr = "@f";
            fileStr += dfsFile.locked ? "L " : "  ";
            fileStr += dfsFile.directory;
            fileStr += '.';

            // Pad file name to the maximum of 12 characters
            string paddedName = dfsFile.name;
            if( paddedName.length() < 11 )
            {
                paddedName.insert( paddedName.end(), 11 - paddedName.length(), ' ');
            }

            fileStr += paddedName;

            // Convert and pad size to a maximum of 6 characters
            string paddedSize = to_string( dfsFile.fileSize );
            if( paddedSize.length() < 6 )
            {
                paddedSize.insert( paddedSize.begin(), 6 - paddedSize.length(), ' ' );
            }
            fileStr += paddedSize;
            fileStr += ' ';
            sectorsUsed += dfsFile.fileSize / 256;
            if( 0 != dfsFile.fileSize % 256 )
            {
                ++sectorsUsed;
            }

            // Convert to hex and pad load address to a maximum of 4 characters
            strStream.str("");
            strStream << hex << dfsFile.loadAddress;
            string paddedLoad = strStream.str();
            if( paddedLoad.length() < 5 )
            {
                paddedLoad.insert( paddedLoad.begin(), 5 - paddedLoad.length(), '0' );
            }
            transform( paddedLoad.begin(), paddedLoad.end(), paddedLoad.begin(), ::toupper );
            fileStr += paddedLoad;
            fileStr += ' ';

            // Convert to hex and pad load address to a maximum of 4 characters
            strStream.str("");
            strStream << hex << dfsFile.execAddress;
            std::string paddedExec = strStream.str();
            if( paddedExec.length() < 5 )
            {
                paddedExec.insert( paddedExec.begin(), 5 - paddedExec.length(), '0' );
            }
            transform( paddedExec.begin(), paddedExec.end(), paddedExec.begin(), ::toupper );
            fileStr += paddedExec;
            fileStr += ' ';

            // Add line to widget
            mDiskContent->add( fileStr.c_str() );
        }

        mDiskContent->add(    "@f-+-+----------+------+-----+-----");
        string freeSpaceStr = "@f    Free Space ";
        string paddedFreeSpace = to_string( (disk.sectorsNum - min(sectorsUsed, (size_t)disk.sectorsNum)) * 256 );
        if( paddedFreeSpace.length() < 6 )
        {
            paddedFreeSpace.insert( paddedFreeSpace.begin(), 6 - paddedFreeSpace.length(), ' ' );
        }
        freeSpaceStr += paddedFreeSpace;
        mDiskContent->add( freeSpaceStr.c_str() );
    }
}

CMMBETable::CMMBETable( CMMBFile* _mmb, int _x, int _y, int _w, int _h, const char* _label ) : Fl_Table( _x, _y, _w, _h, _label )
{
    mIconEmpty    = new Fl_Pixmap( iconEmpty );
    mIconLocked   = new Fl_Pixmap( iconLocked );
    mIconUnlocked = new Fl_Pixmap( iconUnlocked );
    mMMB          = _mmb;
}

CMMBETable::~CMMBETable()
{

}

void CMMBETable::DrawHeader( const char* _string, int _x, int _y, int _w, int _h )
{
    fl_push_clip( _x, _y, _w, _h );
    fl_draw_box( FL_THIN_UP_BOX, _x, _y, _w, _h, row_header_color() );
    fl_color( FL_BLACK );
    fl_draw( _string, _x, _y, _w, _h, FL_ALIGN_CENTER );
    fl_pop_clip();
} 

void CMMBETable::DrawData( const char* _string, Fl_Pixmap* _icon, int _x, int _y, int _w, int _h, bool _selected )
{
    fl_push_clip( _x, _y, _w, _h );

    // Draw cell bg
    fl_color(_selected ? FL_YELLOW : FL_WHITE); 
    fl_rectf( _x, _y, _w, _h );
    
    // Draw cell icon
    if( nullptr != _icon )
    {
        _icon->draw( _x + 1, _y + 1 );
    }

    // Draw cell data
    fl_color(FL_GRAY0); 
    fl_draw( _string, _x+26, _y, _w, _h, FL_ALIGN_LEFT );
    
    // Draw box border
    fl_color( color() );
    fl_rect( _x, _y, _w, _h );
    
    fl_pop_clip();
}

void CMMBETable::DrawUnused( int _x, int _y, int _w, int _h )
{
    fl_color(FL_GRAY); 
    fl_rectf( _x, _y, _w, _h );

    fl_color(FL_GRAY0); 
    fl_rect( _x, _y, _w, _h );
}

int CMMBETable::handle( int _event )
{
    // Compute the targeted slot
    int row = 0;
    int col = 0;
    ResizeFlag rf;
    cursor2rowcol ( row, col, rf );
    size_t slot = (col * rows()) + row;

    EMMBETable_SelectionType selectionType = EMMBETable_Single;
    if( Fl::event_shift() )
    {
        selectionType = EMMBETable_Multiple;
    }
    else if( Fl::event_command() | Fl::event_ctrl() )
    {
        selectionType = EMMBETable_SingleAdd;
    }

    switch( _event )
    {
        case FL_RELEASE:
        {
            if( Fl::event_x() < tix || Fl::event_x() >= tix + tiw ||
                Fl::event_y() < tiy || Fl::event_y() >= tiy + tih )
            {
                return Fl_Table::handle( _event );
            }

            switch( Fl::event_button() )
            {
                case FL_LEFT_MOUSE:
                    SelectSlot( slot, selectionType );
                    break;
                default:
                    break;
            }
            return 1;
        }
        break;
        // Receive a dropped item
        case FL_DND_ENTER:
        case FL_DND_RELEASE:
        case FL_DND_LEAVE:
        case FL_DND_DRAG:
            return 1;

        case FL_PASTE:
        {
            if( nullptr == mMMB || slot >= mMMB->GetNumberOfDisks() )
            {
                return 1;
            }

            std::string pastedText = Fl::event_text();

            // Check for file:// protocol
            std::string fileProtocol = "file://";
            if( 0 == pastedText.compare(0, fileProtocol.length(), fileProtocol) )
            {
                pastedText = pastedText.substr( fileProtocol.length(), std::string::npos );
            }

            // Remove unwanted characters...
            pastedText.erase(std::remove(pastedText.begin(), pastedText.end(), 0xD), pastedText.end());
            pastedText.erase(std::remove(pastedText.begin(), pastedText.end(), 0xA), pastedText.end());

            std::string errorString;
            if( !mMMB->InsertImageInSlot( pastedText, slot, errorString ) )
            {
                fl_alert("[ERROR] %s",errorString.c_str());
            }

            SelectSlot( slot, EMMBETable_Single );

            return 1;
        }
        break;
    }

    return Fl_Table::handle( _event );
}

void CMMBETable::draw_cell( TableContext context, int _row, int _col, int _x, int _y, int _w, int _h )
{
    string tmp;
    char cellData[32] = {0};
    size_t slot = (_col*32)+_row;
    bool selected = IsSlotSelected( slot );

    switch ( context ) 
    {
    case CONTEXT_STARTPAGE:                   // before page is drawn..
        fl_font(FL_COURIER, 16);              // set the font for our drawing operations
        return; 
    case CONTEXT_COL_HEADER:                  // Draw column headers
        tmp = to_string( _col * 32 );
        DrawHeader( tmp.c_str(), _x, _y, _w, _h );
        return; 
    case CONTEXT_ROW_HEADER:                  // Draw row headers
        tmp = to_string( _row );
        DrawHeader( tmp.c_str(), _x, _y, _w, _h );
        return; 
    case CONTEXT_CELL:                        // Draw data in cells
        if( _row == 31 && _col == 15 )
        {
            DrawUnused( _x, _y, _w, _h );
        }
        else
        {
            Fl_Pixmap* cellIcon = nullptr;

            if( nullptr != mMMB )
            {
                sprintf( cellData, "%03zu: %s", slot, mMMB->GetEntryName( slot ) );

                switch( mMMB->GetEntryAttribute( slot ) )
                {
                    case MMB_DISKATTRIBUTE_UNFORMATTED:
                        cellIcon = mIconEmpty;
                        DrawData( cellData, cellIcon, _x, _y, _w, _h, selected );
                        break;
                    case MMB_DISKATTRIBUTE_UNLOCKED:
                        cellIcon = mIconUnlocked;
                        DrawData( cellData, cellIcon, _x, _y, _w, _h, selected );
                        break;
                    case MMB_DISKATTRIBUTE_LOCKED:
                        cellIcon = mIconLocked;
                        DrawData( cellData, cellIcon, _x, _y, _w, _h, selected );
                        break;
                    default:
                        DrawUnused( _x, _y, _w, _h );
                        break;
                }

            }
            else
            {
                sprintf( cellData, "%03i: %s", (_col*32)+_row, "" );
            }
        }
        return;
    default:
        return;
    }
}

void CMMBETable::SelectSlot( size_t _slot, EMMBETable_SelectionType _selectionType )
{
    if( nullptr == mMMB || _slot >= mMMB->GetNumberOfDisks() )
    {
        mLastSelectedSlot = (size_t)-1;
        mSelectedSlots.clear();
        ((CAppWindow*)this->window())->RefreshDiskContent( nullptr, 0 );
    }
    else
    {
        if( _selectionType == EMMBETable_Single )
        {
            // Single click.
            // Deselect all other slots, select the clicked one.
            size_t selectionSize = GetSelectionSize();
            if( selectionSize > 1 || selectionSize == 0 )
            {
                mSelectedSlots.clear();
                AddSlotToSelection( _slot );
            }
            else if( selectionSize == 1 )
            {
                if( IsSlotSelected( _slot ) )
                {
                    RemoveSlotFromSelection( _slot );
                }
                else
                {
                    mSelectedSlots.clear();
                    AddSlotToSelection( _slot );
                }
            }
        }
        else if( _selectionType == EMMBETable_SingleAdd )
        {
            // Single click with Command or Control keys pressed.
            // Change the clicked widget state.
            if( IsSlotSelected( _slot ) )
            {
                RemoveSlotFromSelection( _slot );
            }
            else
            {
                AddSlotToSelection( _slot );
            }
        }
        else // EMMBETable_Multiple
        {
            // Selects range
            if( mLastSelectedSlot != (size_t)-1 && mLastSelectedSlot != _slot )
            {
                mSelectedSlots.clear();

                size_t startSlot = min( mLastSelectedSlot, _slot );
                size_t endSlot   = max( mLastSelectedSlot, _slot );

                for( ; startSlot <= endSlot; ++startSlot )
                {
                    if( !IsSlotSelected(startSlot) )
                    {
                        mSelectedSlots.push_back( startSlot );
                    }
                }
            }
        }
    }

    // Refresh contents
    mLastSelectedSlot = _slot;

    if( mSelectedSlots.size() == 1 )
    {
        string errorString;
        unsigned char* data = new unsigned char[MMB_DISKSIZE];
        mMMB->ExtractImageInSlot( data, mSelectedSlots[0], errorString );
        ((CAppWindow*)this->window())->RefreshDiskContent( data, MMB_DISKSIZE );
        delete[] data;
    }
    else
    {
        ((CAppWindow*)this->window())->RefreshDiskContent( nullptr, 0 );
    }

    redraw();
}

bool CMMBETable::IsSlotSelected( size_t _slot )
{
    return std::find( mSelectedSlots.begin(), mSelectedSlots.end(), _slot ) != mSelectedSlots.end();
}

void CMMBETable::AddSlotToSelection( size_t _slot )
{
    vector<size_t>::const_iterator slotIterator = std::find( mSelectedSlots.begin(), mSelectedSlots.end(), _slot );
    if( slotIterator == mSelectedSlots.end() )
    {
        mSelectedSlots.push_back( _slot );
    }
}

void CMMBETable::RemoveSlotFromSelection( size_t _slot )
{
    vector<size_t>::const_iterator slotIterator = std::find( mSelectedSlots.begin(), mSelectedSlots.end(), _slot );
    if( slotIterator != mSelectedSlots.end() )
    {
        mSelectedSlots.erase( slotIterator );
    }
}

void CMMBETable::DoRedraw()
{
    redraw();
}

size_t CMMBETable::GetSelectionSize()
{
    return mSelectedSlots.size();
}

const std::vector<size_t>& CMMBETable::GetSelection()
{
    return mSelectedSlots;
}

CMMBEGui::CMMBEGui( int _w, int _h, const char* _label )
{
    Fl::visual(FL_RGB);

    mMainWindow = new CAppWindow( _w, _h + mMenuBarOffset, _label );

    mMainWindow->begin();

    CreateMenuBar();

    CreateControls();

    mMainWindow->end();

#ifndef __APPLE__
#ifdef WIN32
	mMainWindow->icon((char*)LoadIcon(fl_display, MAKEINTRESOURCE(101)));
#else
    Fl_Pixmap xpmIcon( mmbexplorer_64x64 );
    Fl_RGB_Image appIcon( &xpmIcon, Fl_Color(0) );
    mMainWindow->icon( &appIcon );
#endif
#endif

    mMainWindow->resizable( mTable );
}

CMMBEGui::~CMMBEGui()
{

}

int CMMBEGui::Run()
{
    if( nullptr != mMainWindow )
    {
        mMainWindow->show(0, nullptr);
        mMainWindow->redraw();
        return Fl::run();
    }

    return -1;
}

void CMMBEGui::OpenMMB( const std::string& _filename )
{
    std::string errorString;

    if( !mMMB.Open( _filename, errorString ) )
    {
        fl_alert("[ERROR] %s",errorString.c_str());
    }

    string filenameStr = "File: ";
    filenameStr += mMMB.GetFilename().c_str();
    mFilenameBox->copy_label( filenameStr.c_str() );

    // Refresh contents
    mTable->SelectSlot( 0, CMMBETable::EMMBETable_Single );
    mTable->redraw();
}

void CMMBEGui::CreateMMB( const std::string& _filename )
{
    std::string errorString;

    CloseMMB();

    // Ask for MMB size
    const char* result = fl_input( "Enter number of slots on the new MMB file (1 - 511)", nullptr );
    if( nullptr == result )
    {
        return;
    }

    size_t numberOfDisks = (size_t)strtoul( result, nullptr, 0 );
    if( numberOfDisks == 0 || numberOfDisks > 511 )
    {
        return;
    }

    if( !mMMB.Create( _filename, numberOfDisks, errorString ) )
    {
        fl_alert("[ERROR] %s",errorString.c_str());

        string filenameStr = "File: ";
        mFilenameBox->copy_label( filenameStr.c_str() );
        mTable->redraw();
        return;
    }

    OpenMMB( _filename );

    // Refresh contents
    mTable->redraw();
}

void CMMBEGui::CloseMMB()
{
    mMMB.Close();

    string filenameStr = "File: ";
    mFilenameBox->copy_label( filenameStr.c_str() );

    mTable->SelectSlot( MMB_MAXNUMBEROFDISKS, CMMBETable::EMMBETable_Single );

    // Refresh contents
    mTable->redraw();
}

void CMMBEGui::CreateMenuBar()
{
	// Menu bar
	#ifdef __APPLE__
	mMenuBar = new Fl_Sys_Menu_Bar(0,0,mMainWindow->w(),30);
	#else
	mMenuBar = new Fl_Menu_Bar(0,0,mMainWindow->w(),30);
	#endif

	// File menu
	mMenuBar->add( "&File/&Open MMB"      , mModifierKey+'o', openFile_cb  , (void*)this, 0 );
	mMenuBar->add( "&File/&Close MMB"     , mModifierKey+'c', closeFile_cb , (void*)this, 0 );
	mMenuBar->add( "&File/Create &new MMB", mModifierKey+'n', createFile_cb, (void*)this, 0 );
	#ifndef __APPLE__
    mMenuBar->add( "&File/&Quit"          , mModifierKey+'q', menuQuit_cb  , (void*)mMainWindow, 0 );
    #endif

    // Slot menu
	mMenuBar->add( "&Slot/&Insert disk"   , mModifierKey+'i', insertDisk_cb  , (void*)this, 0 ); // If no slot selected, ask for slot
	mMenuBar->add( "&Slot/&Extract disk"  , mModifierKey+'e', extractDisk_cb , (void*)this, 0 );
	mMenuBar->add( "&Slot/&Remove disk"   , mModifierKey+'r', removeDisk_cb  , (void*)this, 0 );
	mMenuBar->add( "&Slot/&Lock disk"     , mModifierKey+'l', lockDisk_cb    , (void*)this, 0 );
	mMenuBar->add( "&Slot/&Unlock disk"   , mModifierKey+'u', unlock_cb      , (void*)this, 0 );

    // Help menu
	#ifndef __APPLE__
    mMenuBar->add( "&Help/&About"         , mModifierKey+'a', menuAbout_cb  , (void*)mMainWindow, 0 );
    #endif
}

void CMMBEGui::CreateControls()
{
    int x = 10;
    int y = 10 + mMenuBarOffset;

    // File name
    mFilenameBox = new Fl_Box( 10, y, 620, 26, "File: " );
    mFilenameBox->align( FL_ALIGN_INSIDE | FL_ALIGN_LEFT );
    y += 21;

    mTable = new CMMBETable( &mMMB, 10, y, 472, 384, 0 );
    mTable->begin();

    // Rows
    mTable->rows(32);             // how many rows
    mTable->row_header(0);        // enable row headers (along left)
    mTable->row_height_all(26);   // default height of rows
    mTable->row_resize(0);        // disable row resizing
    
    // Columns
    mTable->cols(16);             // how many columns
    mTable->col_header(0);        // enable column headers (along top)
    mTable->col_width_all(206);   // default width of columns
    mTable->col_resize(0);        // enable column resizing
    mTable->end();			      // end the Fl_Table group

    x += 472 + 10;
    Fl_Select_Browser* diskContent = new Fl_Select_Browser( x, y, 300, 384, "Disk content" );
	diskContent->color( FL_WHITE );
    diskContent->align( FL_ALIGN_TOP );
	diskContent->type(FL_HOLD_BROWSER);
    mMainWindow->SetDiskContentWidget( diskContent );
}

size_t CMMBEGui::GetNumberOfSlots() const
{
    return mMMB.GetNumberOfDisks();    
}

void CMMBEGui::InsertDisk ( const std::string& _filename, size_t _slot )
{
    std::string errorString;

    if( !mMMB.InsertImageInSlot( _filename, _slot, errorString ) )
    {
        fl_alert( "[ERROR] %s",errorString.c_str() );
    }
    else
    {
        if( GetSelectionSize() > 1 )
        {
            mTable->redraw();
            return;
        }

        unsigned char* pData = new unsigned char[MMB_DISKSIZE];
        if( nullptr == pData )
        {
            fl_alert( "[ERROR] Could't allocate %zu bytes for temporal storage.",MMB_DISKSIZE );
            return;
        }

        if( !mMMB.ExtractImageInSlot( pData, _slot, errorString ) )
        {
            delete[] pData;
            fl_alert( "[ERROR] %s", errorString.c_str() );
        }

        if( mTable->IsSlotSelected(_slot) )
        {
            mMainWindow->RefreshDiskContent( pData, MMB_DISKSIZE);    
        }
        else
        {
            mMainWindow->RefreshDiskContent( nullptr, 0);
        }

        delete[] pData;
    }
    

    // Refresh contents
    mTable->redraw();
}

void CMMBEGui::ExtractDisk( const std::string& _filename, size_t _slot )
{
    std::string errorString;

    if( !mMMB.ExtractImageInSlot( _filename, _slot, errorString ) )
    {
        fl_alert("[ERROR] %s",errorString.c_str());        
    }

    // Refresh contents
    mTable->redraw();
}

void CMMBEGui::RemoveDisk ( size_t _slot )
{
    std::string errorString;

    if( !mMMB.RemoveImageFromSlot( _slot, errorString ) )
    {
        fl_alert("[ERROR] %s",errorString.c_str());        
    }
    else
    {
        mMainWindow->RefreshDiskContent( nullptr, 0 );
    }

    // Refresh contents
    mTable->redraw();
}

void CMMBEGui::LockDisk   ( size_t _slot )
{
    std::string errorString;

    if( !mMMB.LockImageInSlot( _slot, errorString ) )
    {
        fl_alert("[ERROR] %s",errorString.c_str());        
    }

    // Refresh contents
    mTable->redraw();
}

void CMMBEGui::UnlockDisk ( size_t _slot )
{
    std::string errorString;

    if( !mMMB.UnlockImageInSlot( _slot, errorString ) )
    {
        fl_alert("[ERROR] %s",errorString.c_str());        
    }

    // Refresh contents
    mTable->redraw();
}

void CMMBEGui::ExtractSelectedDisks()
{
    if( GetSelectionSize() == 0 )
    {
        size_t slot = ChooseSlot();
		std::string filename;
		if( !ChooseFilename( filename, "SSD files\t*.ssd\n", ".", true) )
			return;

		ExtractDisk( filename, slot );
    }
    else if( GetSelectionSize() == 1 )
    {
   		std::string filename;
		if( !ChooseFilename( filename, "SSD files\t*.ssd\n", ".", true) )
			return;

        ExtractDisk( filename, mTable->GetSelection()[0] );
    }
    else
    {
        // Choose export folder
        string folderName;
        if( !ChooseFilename( folderName, "", "", true, true ) )
            return;

    #ifdef WIN32
        if( folderName.back() != '\\' ) folderName += '\\';
    #else
        if( folderName.back() != '/' ) folderName += '/';
    #endif

        for( auto slot : mTable->GetSelection() )
        {
            unsigned char diskAttribute = mMMB.GetEntryAttribute(slot);
            if( MMB_DISKATTRIBUTE_INVALID == diskAttribute || MMB_DISKATTRIBUTE_UNFORMATTED == diskAttribute )
            {
                continue;
            }

            string filename = folderName;

            string slotStr = to_string(slot);
            if( slotStr.length() < 3 )
            {
                slotStr.insert( slotStr.begin(), 3 - slotStr.length(), '0');
            }
            filename += slotStr;
            filename += '_';
            filename += mMMB.GetEntryName( slot );
            filename += ".ssd";

            ExtractDisk( filename, slot );
        }
    }
}

void CMMBEGui::RemoveSelectedDisks()
{
    for( auto slot : mTable->GetSelection() )
    {
        RemoveDisk( slot );
    }
}

void CMMBEGui::LockSelectedDisks()
{
    for( auto slot : mTable->GetSelection() )
    {
        LockDisk( slot );
    }
}

void CMMBEGui::UnlockSelectedDisks()
{
    for( auto slot : mTable->GetSelection() )
    {
        UnlockDisk( slot );
    }
}

size_t CMMBEGui::GetSelectionSize()
{
    return mTable->GetSelectionSize();
}

void CMMBEGui::ShowAboutDialog()
{
    if( nullptr != Fl::modal() )
    {
        return;
    }

    int aboutDialogWidth  = 284;
    int aboutDialogHeight = 195;

    Fl_Window* aboutDialog = new Fl_Window( aboutDialogWidth, aboutDialogHeight, nullptr );
    if( nullptr == aboutDialog )
    {
        return;
    }
#ifdef WIN32
    aboutDialog->resizable( aboutDialog );
#endif
    aboutDialog->box(FL_FLAT_BOX);
    aboutDialog->color( FL_DARK2 );
    aboutDialog->set_modal();
    aboutDialog->begin();

    int ySeparation = 8;
    int y = ySeparation;

    Fl_Pixmap* aboutIcon = new Fl_Pixmap( mmbexplorer_64x64 );
    Fl_Box* aboutIconBox = new Fl_Box( (aboutDialogWidth - aboutIcon->w()) / 2, y, aboutIcon->w(), aboutIcon->h() );
    aboutIconBox->image(aboutIcon);
    y += aboutIcon->h() + ySeparation;

    int textWidth  = 104;
    int textHeight = 15;

    Fl_Box* aboutText1 = new Fl_Box( (aboutDialogWidth - textWidth)/2, y, textWidth, textHeight, "MMBExplorer" );
    aboutText1->align( FL_ALIGN_CENTER );
    aboutText1->labelcolor( FL_BLACK );
    aboutText1->labelsize( 14 );
    aboutText1->labelfont( FL_HELVETICA_BOLD );
    y += textHeight + ySeparation;

    std::string versionStr = "Version 1.0 (";
    versionStr += GetSCMVersion();
    versionStr += ")";
    Fl_Box* aboutText2 = new Fl_Box( (aboutDialogWidth - textWidth)/2, y, textWidth, textHeight, nullptr );
    aboutText2->align( FL_ALIGN_CENTER );
    aboutText2->labelcolor( FL_BLACK );
    aboutText2->labelsize( 12 );
    aboutText2->labelfont( FL_HELVETICA );
    aboutText2->copy_label( versionStr.c_str() );
    y += textHeight + ySeparation;

    std::string fltkVersionStr = "GUI with FLTK ";
    fltkVersionStr += std::to_string( FL_MAJOR_VERSION );
    fltkVersionStr += ".";
    fltkVersionStr += std::to_string( FL_MINOR_VERSION );
    Fl_Box* aboutText3 = new Fl_Box( (aboutDialogWidth - textWidth)/2, y, textWidth, textHeight, "GUI with FLTK 1.3" );
    aboutText3->align( FL_ALIGN_CENTER );
    aboutText3->labelcolor( FL_BLACK );
    aboutText3->labelsize( 12 );
    aboutText3->labelfont( FL_HELVETICA_BOLD );
    y += textHeight + ySeparation;

    Fl_Box* aboutText4 = new Fl_Box( (aboutDialogWidth - textWidth)/2, y, textWidth, textHeight * 2, "Copyright 2020 by Roberto Carlos\nFernández Gerhardt" );
    aboutText4->align( FL_ALIGN_CENTER );
    aboutText4->labelcolor( FL_BLACK );
    aboutText4->labelsize( 12 );
    aboutText4->labelfont( FL_HELVETICA );

    aboutDialog->end();
    aboutDialog->show();
}

const std::vector<size_t>& CMMBEGui::GetSelection()
{
    return mTable->GetSelection();
}