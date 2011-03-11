/* This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import "ExpressController.h"

#define INSERT_STRING       @"Insert a DVD"
#define TOOLBAR_START       @"TOOLBAR_START"
#define TOOLBAR_PAUSE       @"TOOLBAR_PAUSE"
#define TOOLBAR_OPEN        @"TOOLBAR_OPEN"
#define TOOLBAR_ADVANCED    @"TOOLBAR_ADVANCED"

#define p fState->param

#import "Device.h"

@interface ExpressController (Private)

- (void) openUpdateDrives: (NSDictionary *) drives;
- (void) openBrowseDidEnd: (NSOpenPanel *) sheet returnCode: (int)
    returnCode contextInfo: (void *) contextInfo;
- (void) openEnable: (BOOL) b;

- (id) updatePopUpIcon: (id) value;
- (void) convertShow;
- (void) convertEnable: (BOOL) b;

@end

@implementation ExpressController

/***********************************************************************
 * Application delegate methods
 **********************************************************************/
- (void) awakeFromNib
{
    NSEnumerator * enumerator;
    
    /* NSToolbar initializations */
    fToolbar = [[NSToolbar alloc] initWithIdentifier: @"InstantHandBrake Toolbar"];
    [fToolbar setDelegate: self];
    [fToolbar setAllowsUserCustomization: NO];
    [fToolbar setDisplayMode: NSToolbarDisplayModeIconAndLabel];
    [fToolbar setVisible:NO];
    [fWindow setShowsToolbarButton:NO];
    [fWindow setToolbar: fToolbar];
        
    /* Show the "Open DVD" interface */
    fDriveDetector = [[DriveDetector alloc] initWithCallback: self
        selector: @selector( openUpdateDrives: )];
    [fDriveDetector run];
    [self openEnable: YES];
    [fWindow setContentSize: [fOpenView frame].size];
    [fWindow setContentView: fOpenView];
    [fWindow center];
    [fWindow makeKeyAndOrderFront: nil];

    /* NSTableView initializations */
     NSButtonCell * buttonCell;
     NSTableColumn * tableColumn;
     enumerator = [[fConvertTableView tableColumns] objectEnumerator];
     while( ( tableColumn = [enumerator nextObject] ) )
     {
         [tableColumn setEditable: NO];
     }
     tableColumn = [fConvertTableView tableColumnWithIdentifier: @"Check"];
     buttonCell = [[[NSButtonCell alloc] initTextCell: @""] autorelease];
    [buttonCell setEditable: YES];
    [buttonCell setButtonType: NSSwitchButton];
    [tableColumn setDataCell: buttonCell];

    /* Preferences */
    fConvertFolderString = [NSHomeDirectory() stringByAppendingPathComponent:@"Desktop"];
    [fConvertFolderString retain];
}

- (void) applicationWillFinishLaunching: (NSNotification *) n
{
    fCore = [[HBCore alloc] init];
    [fCore openInDebugMode:NO checkForUpdates:NO];
    fHandle = [fCore hb_handle];
    fState = [fCore hb_state];
    fList   = hb_get_titles( fHandle );
    
    fDevice = [[DeviceController alloc] init];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
    selector:@selector(scanningSource:)
    name:@"HBCoreScanningNotification" object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
    selector:@selector(scanDone:)
    name:@"HBCoreScanDoneNotification" object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
    selector:@selector(muxing:)
    name:@"HBCoreMuxingNotification" object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
    selector:@selector(working:)
    name:@"HBCoreWorkingNotification" object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
    selector:@selector(workDone:)
    name:@"HBCoreWorkDoneNotification" object:nil];
    
    [GrowlApplicationBridge setGrowlDelegate: self];
}

- (void) applicationWillTerminate: (NSNotification *) n
{
    [fCore close];
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag
{
    if( !flag ) {
        [fWindow  makeKeyAndOrderFront:nil];
        return YES;
    }
    return NO;
}

- (NSToolbarItem *) toolbar: (NSToolbar *) toolbar itemForItemIdentifier: (NSString *) ident
                    willBeInsertedIntoToolbar: (BOOL) flag
{
    NSToolbarItem * item;
    item = [[NSToolbarItem alloc] initWithItemIdentifier: ident];

    if ([ident isEqualToString: TOOLBAR_START])
    {
        [item setLabel: NSLocalizedString(@"Convert", "Convert")];
        [item setPaletteLabel: NSLocalizedString(@"Convert/Cancel", @"Convert/Cancel")];
        [item setImage: [NSImage imageNamed: @"Play"]];
        [item setTarget: self];
        [item setAction: @selector(convertGo:)];
    }
    else if ([ident isEqualToString: TOOLBAR_PAUSE])
    {
        [item setLabel: NSLocalizedString(@"Pause", "Pause")];
        [item setPaletteLabel: NSLocalizedString(@"Pause/Resume", @"Pause/Resume")];
        [item setImage: [NSImage imageNamed: @"Pause"]];
        [item setTarget: self];
        [item setAction: @selector(pauseGo:)];
    }
    else if ([ident isEqualToString: TOOLBAR_OPEN])
    {
        [item setLabel: NSLocalizedString(@"Open...", "Open...")];
        [item setPaletteLabel: NSLocalizedString(@"Open Another Source", "Open Another Source")];
        [item setImage: [NSImage imageNamed: @"Open"]];
        [item setTarget: self];
        [item setAction: @selector(openShow:)];
    }
    else
    {
        [item release];
        return nil;
    }

    return item;
}

- (NSArray *) toolbarDefaultItemIdentifiers: (NSToolbar *) toolbar
{
    return [NSArray arrayWithObjects: TOOLBAR_START, TOOLBAR_PAUSE,
                                    NSToolbarFlexibleSpaceItemIdentifier, TOOLBAR_OPEN, nil];
}

- (NSArray *) toolbarAllowedItemIdentifiers: (NSToolbar *) toolbar
{
    return [NSArray arrayWithObjects: TOOLBAR_START, TOOLBAR_PAUSE,
                                        TOOLBAR_OPEN, NSToolbarFlexibleSpaceItemIdentifier, nil];
}

/***********************************************************************
 * Tableview datasource methods
 **********************************************************************/
- (int) numberOfRowsInTableView: (NSTableView *) t
{
    if( !fHandle )
        return 0;

    return hb_list_count( fList );
}

- (id) tableView:(NSTableView *) t objectValueForTableColumn:
    (NSTableColumn *) col row: (int) row
{
    if( [[col identifier] isEqualToString: @"Check"] )
    {
        return [fConvertCheckArray objectAtIndex: row];
    }
    else
    {
        hb_title_t * title = hb_list_item( fList, row );
        if( [[col identifier] isEqualToString: @"Title"] )
        {
            return [@"Title " stringByAppendingFormat: @"%d",
                    title->index];
        }
        else if( [[col identifier] isEqualToString: @"Duration"] )
        {
            if( title->hours > 0 )
            {
                return [NSString stringWithFormat:
                    @"%d hour%s %d min%s", title->hours,
                    title->hours > 1 ? "s" : "", title->minutes,
                    title->minutes > 1 ? "s": ""];
            }
            else if( title->minutes > 0 )
            {
                return [NSString stringWithFormat:
                    @"%d min%s %d sec%s", title->minutes,
                    title->minutes > 1 ? "s" : "", title->seconds,
                    title->seconds > 1 ? "s": ""];
            }
            else
            {
                return [NSString stringWithFormat: @"%d seconds",
                        title->seconds];
            }
        }
        else if( [[col identifier] isEqualToString: @"Size"] )
        {
            return [NSString stringWithFormat:@"-"];
        }
    }
    return nil;
}

- (void) tableView: (NSTableView *) t setObjectValue: (id) object
    forTableColumn: (NSTableColumn *) col row: (int) row
{
    if( [[col identifier] isEqualToString: @"Check"] )
    {
        [fConvertCheckArray replaceObjectAtIndex: row withObject: object];
    }
}

/***********************************************************************
 * User events methods
 **********************************************************************/
- (void) openShow: (id) sender
{
    NSRect frame  = [fWindow frame];
    float  offset = ( [fConvertView frame].size.height -
                    [fOpenView frame].size.height ) * [fWindow userSpaceScaleFactor];

    frame.origin.y    += offset;
    frame.size.height -= offset;
    [fWindow setContentView: fEmptyView];
    [fWindow setFrame: frame display: YES animate: YES];
    [fToolbar setVisible:NO];
    [fOpenProgressField setStringValue: @""];
    [fWindow setContentView: fOpenView];

    [fDriveDetector run];
}

- (void) openMatrixChanged: (id) sender
{
    [self openEnable: YES];
    if( [fOpenMatrix selectedRow] )
    {
        [self openBrowse: self];
    }
}

- (void) openBrowse: (id) sender
{
    NSOpenPanel * panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection: NO];
    [panel setCanChooseFiles: YES];
    [panel setCanChooseDirectories: YES ];
    [panel beginSheetForDirectory: nil file: nil types: nil
        modalForWindow: fWindow modalDelegate: self
        didEndSelector: @selector( openBrowseDidEnd:returnCode:contextInfo: )
        contextInfo: nil];                                                      
}

- (void) openGo: (id) sender
{
    [self openEnable: NO];
    [fOpenIndicator setIndeterminate: YES];
    [fOpenIndicator startAnimation: nil];
    [fOpenProgressField setStringValue: NSLocalizedString( @"Opening...", @"Opening...") ];
    [fDriveDetector stop];

    if( [fOpenMatrix selectedRow] )
    {
        hb_scan( fHandle, [fOpenFolderString UTF8String], 0 );
    }
    else
    {
        hb_scan( fHandle, [[fDrives objectForKey: [fOpenPopUp
                 titleOfSelectedItem]] UTF8String], 0 );
    }
}

- (void) selectFolderSheetShow: (id) sender
{
    NSOpenPanel * panel = [NSOpenPanel openPanel];

    [panel setPrompt: NSLocalizedString(@"Select", @"Convert -> Save panel prompt")];
    [panel setAllowsMultipleSelection: NO];
    [panel setCanChooseFiles: NO];
    [panel setCanChooseDirectories: YES];
    [panel setCanCreateDirectories: YES];

    [panel beginSheetForDirectory: nil file: nil types: nil
        modalForWindow: fWindow modalDelegate: self didEndSelector:
        @selector(selectFolderSheetClosed:returnCode:contextInfo:) contextInfo: nil];
}

- (void) selectFolderSheetClosed: (NSOpenPanel *) sheet returnCode: (int)
    returnCode contextInfo: (void *) contextInfo
{
    if( returnCode != NSOKButton )
        return;

    if( fConvertFolderString )
        [fConvertFolderString release];
    fConvertFolderString = [[[sheet filenames] objectAtIndex: 0] retain];
    [[fConvertFolderPopUp itemAtIndex: 0] setTitle: [fConvertFolderString lastPathComponent]];
    [fConvertFolderPopUp selectItemAtIndex:0];
    
    NSMenuItem * item = [fConvertFolderPopUp itemAtIndex: 0];
    [item setImage: [self updatePopUpIcon:fConvertFolderString]];
    
}

- (void) convertGo: (id) sender
{
    int i, j;
    Preset * currentPreset = [[[fDevice devicesList] objectAtIndex:[fConvertFormatPopUp indexOfSelectedItem]] firstPreset];

    for( i = 0; i < hb_list_count( fList ); i++ )
    {
        if( ![[fConvertCheckArray objectAtIndex: i] boolValue] )
            continue;

        hb_title_t * title = hb_list_item( fList, i );
        hb_job_t   * job   = title->job;

		int maxwidth = [currentPreset maxWidth];
        int maxheight = [currentPreset maxHeight];
        int pixels = maxwidth * maxheight;
		int aspect = title->aspect;
        
		if( [fConvertAspectPopUp indexOfSelectedItem] == 1 )
		{
            aspect = 4 * HB_ASPECT_BASE / 3;
		}
        else if ( [fConvertAspectPopUp indexOfSelectedItem] == 2 )
        {
            aspect = 16 * HB_ASPECT_BASE / 9;
        }

		job->vbitrate = [currentPreset videoBitRate];
		
        if( [fConvertMaxWidthPopUp indexOfSelectedItem] == 2 )
		{
            maxwidth = 480;
			job->vbitrate /= 1.5;
        }
        else if ( [fConvertMaxWidthPopUp indexOfSelectedItem] == 3 )
        {
            maxwidth = 320;
			job->vbitrate /= 2;
        }
        
        if ( [fConvertAspectPopUp indexOfSelectedItem] > 0 )
        {
            do
            {
                hb_set_size( job, aspect, pixels );
                pixels -= 10;
            } while(job->width > maxwidth || job->height > maxheight);
        }
        else
        {
            /* Reset job->crop values */
            memcpy( job->crop, job->title->crop, 4 * sizeof( int ) );
            job->width = maxwidth;
            hb_fix_aspect( job, HB_KEEP_WIDTH );
        }
		
        job->mux        = [currentPreset muxer];
        job->vcodec     = [currentPreset videoCodec];
        job->advanced_opts = (char *)calloc(1024, 1); /* Fixme, this just leaks */  
        strcpy(job->advanced_opts, [[currentPreset videoCodecOptions] UTF8String]);
        job->chapter_markers = 1;
        job->vquality = -1.0;

        const char * lang;

        /* Audio selection */
        hb_audio_t * audio;
        lang = [[fConvertAudioPopUp titleOfSelectedItem] UTF8String];
        job->audios[0] = -1;
        for( j = 0; j < hb_list_count( title->list_audio ); j++ )
        {
            /* Choose the first track that matches the language */
            audio = hb_list_item( title->list_audio, j );
            if( !strcmp( lang, audio->lang_simple ) )
            {
                job->audios[0] = j;
                break;
            }
        }
        if( job->audios[0] == -1 )
        {
            /* If the language isn't available in this title, choose
               the first track */
            job->audios[0] = 0;
        }
        job->audios[1] = -1;

        job->audio_mixdowns[0] = HB_AMIXDOWN_DOLBYPLII;
        
        /* Subtitle selection */
        hb_subtitle_t * subtitle;
        lang = [[fConvertSubtitlePopUp titleOfSelectedItem] UTF8String];
        job->subtitle = -1;
        for( j = 0; j < hb_list_count( title->list_subtitle ); j++ )
        {
            /* Choose the first track that matches the language */
            subtitle = hb_list_item( title->list_subtitle, j );
            if( !strcmp( lang, subtitle->lang ) )
            {
                job->subtitle = j;
                break;
            }
        }
        
        job->file = strdup( [[NSString stringWithFormat:                 
                @"%@/%s - Title %d.m4v", fConvertFolderString,      
                title->name, title->index] UTF8String] );
        hb_add( fHandle, job );
    }

    hb_start( fHandle );

    [self convertEnable: NO];
}

- (void) convertCancel: (id) sender
{
    hb_stop( fHandle );
    [self convertEnable: YES];
}

@end

/***********************************************************************
 * Private methods
 **********************************************************************/

@implementation ExpressController (Private)

- (void) openUpdateDrives: (NSDictionary *) drives
{
    if( fDrives )
    {
        [fDrives release];
    }
    fDrives = [[NSDictionary alloc] initWithDictionary: drives];

    NSString * device;
    NSEnumerator * enumerator = [fDrives keyEnumerator];
    [fOpenPopUp removeAllItems];
    while( ( device = [enumerator nextObject] ) )
    {
        [fOpenPopUp addItemWithTitle: device];
    }

    if( ![fOpenPopUp numberOfItems] )
    {
        [fOpenPopUp addItemWithTitle: INSERT_STRING];
    }
    [fOpenPopUp selectItemAtIndex: 0];
    if( [fOpenMatrix isEnabled] )
    {
        [self openEnable: YES];
    }
}

- (void) openBrowseDidEnd: (NSOpenPanel *) sheet returnCode: (int)
    returnCode contextInfo: (void *) contextInfo
{
    if( returnCode != NSOKButton )
        return;

    if( fOpenFolderString )
        [fOpenFolderString release];
    fOpenFolderString = [[[sheet filenames] objectAtIndex: 0] retain];
    [fOpenFolderField setStringValue: [fOpenFolderString lastPathComponent]];
    [self openGo: self];
}

- (BOOL)validateToolbarItem: (NSToolbarItem *) toolbarItem
{
    NSString * ident = [toolbarItem itemIdentifier];
    
    if ([ident isEqualToString: TOOLBAR_START] && [HBStateWorking isEqualToString:[fCore state]])
    {
        [toolbarItem setAction: @selector(convertCancel:)];
        [toolbarItem setLabel:NSLocalizedString(@"Cancel", @"Cancel")];
        return YES;
    }
    else if ([ident isEqualToString: TOOLBAR_START] && [HBStateWorkDone isEqualToString:[fCore state]])
    {
        [toolbarItem setAction: @selector(convertGo:)];
        [toolbarItem setLabel:NSLocalizedString(@"Convert", @"Convert")];
        return YES;
    }
    else if ([ident isEqualToString: TOOLBAR_OPEN] && [HBStateWorking isEqualToString:[fCore state]])
    {
        return NO;
    }
    
    return YES;
}

- (void) openEnable: (BOOL) b
{
    [fOpenMatrix       setEnabled: b];
    [fOpenPopUp        setEnabled: b];
    [fOpenFolderField  setEnabled: b];
    [fOpenBrowseButton setEnabled: b];
    [fOpenGoButton     setEnabled: b]; 

    if( b )
    {
        if( [fOpenMatrix selectedRow] )
        {
            [fOpenPopUp setEnabled: NO];
        }
        else
        {
            [fOpenFolderField  setEnabled: NO];
            [fOpenBrowseButton setEnabled: NO];
            if( [[fOpenPopUp titleOfSelectedItem]
                    isEqualToString: INSERT_STRING] )
            {
                [fOpenGoButton setEnabled: NO];
            }
        }
    }
}

- (void) scanningSource: (NSNotification *) n
{
    [fOpenIndicator setIndeterminate: NO];
    [fOpenIndicator setDoubleValue: 100.0 *
            ( (float) p.scanning.title_cur - 0.5 ) / p.scanning.title_count];
    [fOpenProgressField setStringValue: [NSString
            stringWithFormat: @"Scanning title %d of %d...",
            p.scanning.title_cur, p.scanning.title_count]];
}

- (void) scanDone: (NSNotification *) n
{    
    [fOpenIndicator setIndeterminate: NO];
    [fOpenIndicator setDoubleValue: 0.0];
   
     [self openEnable: YES];

    if( hb_list_count( fList ) )
    {
        [self convertShow];
    }
    else
    {
        [fDriveDetector run];
        [fOpenProgressField setStringValue: NSLocalizedString(@"No Title Found...",@"No Title Found...")];
    }
}

- (void) convertShow
{
    int i, j;

    fConvertCheckArray = [[NSMutableArray alloc] initWithCapacity:
        hb_list_count( fList )];
    [fConvertAudioPopUp removeAllItems];
    [fConvertSubtitlePopUp removeAllItems];
    [fConvertSubtitlePopUp addItemWithTitle: @"None"];
    for( i = 0; i < hb_list_count( fList ); i++ )
    {
        /* Default is to convert titles longer than 30 minutes. */
        hb_title_t * title = hb_list_item( fList, i );
        [fConvertCheckArray addObject: [NSNumber numberWithBool:
            ( 60 * title->hours + title->minutes > 30 )]];

        /* Update audio popup */
        hb_audio_t * audio;
        for( j = 0; j < hb_list_count( title->list_audio ); j++ )
        {
            audio = hb_list_item( title->list_audio, j );
            [fConvertAudioPopUp addItemWithTitle:
                [NSString stringWithUTF8String: audio->lang_simple]];
        }
		[fConvertAudioPopUp selectItemWithTitle: @"English"];
        
        if ( [fConvertAudioPopUp selectedItem] == nil )
            [fConvertAudioPopUp selectItemAtIndex:0];

        /* Update subtitle popup */
        hb_subtitle_t * subtitle;
        for( j = 0; j < hb_list_count( title->list_subtitle ); j++ )
        {
            subtitle = hb_list_item( title->list_subtitle, j );
            [fConvertSubtitlePopUp addItemWithTitle:
                [NSString stringWithUTF8String: subtitle->lang]];
        }
    }
    [fConvertTableView reloadData];
    
    NSEnumerator * enumerator;
    Device * device;
    enumerator = [[fDevice devicesList] objectEnumerator];
    
    while( ( device = [enumerator nextObject] ) )
        [fConvertFormatPopUp addItemWithTitle:[device name]];

    NSRect frame  = [fWindow frame];
    float  offset = ( [fConvertView frame].size.height -
                    [fOpenView frame].size.height ) * [fWindow userSpaceScaleFactor];;
    frame.origin.y    -= offset;
    frame.size.height += offset;
    [fWindow setContentView: fEmptyView];
    [fWindow setFrame: frame display: YES animate: YES];
    [fToolbar setVisible:YES];
    [fWindow setContentView: fConvertView];

    NSMenuItem * item = [fConvertFolderPopUp itemAtIndex: 0];
    [item setTitle: [fConvertFolderString lastPathComponent]];
    [item setImage: [self updatePopUpIcon:fConvertFolderString]];
    
    [self convertEnable: YES];
}

- (void) convertEnable: (BOOL) b
{
    [fConvertTableView setEnabled: b];
    [fConvertFolderPopUp setEnabled: b];
    [fConvertFormatPopUp setEnabled: b];
    [fConvertAspectPopUp setEnabled: b];
    [fConvertMaxWidthPopUp setEnabled: b];
    [fConvertAudioPopUp setEnabled: b];
    [fConvertSubtitlePopUp setEnabled: b];
}

/***********************************************************************
* UpdateDockIcon
***********************************************************************
* Shows a progression bar on the dock icon, filled according to
* 'progress' (0.0 <= progress <= 1.0).
* Called with progress < 0.0 or progress > 1.0, restores the original
* icon.
**********************************************************************/
- (void) UpdateDockIcon: (float) progress
{
    NSImage * icon;
    NSData * tiff;
    NSBitmapImageRep * bmp;
    uint32_t * pen;
    uint32_t black = htonl( 0x000000FF );
    uint32_t red   = htonl( 0xFF0000FF );
    uint32_t white = htonl( 0xFFFFFFFF );
    int row_start, row_end;
    int i, j;
	
    /* Get application original icon */
    icon = [NSImage imageNamed: @"NSApplicationIcon"];
	
    if( progress < 0.0 || progress > 1.0 )
    {
        [NSApp setApplicationIconImage: icon];
        return;
    }
	
    /* Get it in a raw bitmap form */
    tiff = [icon TIFFRepresentationUsingCompression:
					   NSTIFFCompressionNone factor: 1.0];
    bmp = [NSBitmapImageRep imageRepWithData: tiff];
    
    /* Draw the progression bar */
    /* It's pretty simple (ugly?) now, but I'm no designer */
	
    row_start = 3 * (int) [bmp size].height / 4;
    row_end   = 7 * (int) [bmp size].height / 8;
	
    for( i = row_start; i < row_start + 2; i++ )
    {
        pen = (uint32_t *) ( [bmp bitmapData] + i * [bmp bytesPerRow] );
        for( j = 0; j < (int) [bmp size].width; j++ )
        {
            pen[j] = black;
        }
    }
    for( i = row_start + 2; i < row_end - 2; i++ )
    {
        pen = (uint32_t *) ( [bmp bitmapData] + i * [bmp bytesPerRow] );
        pen[0] = black;
        pen[1] = black;
        for( j = 2; j < (int) [bmp size].width - 2; j++ )
        {
            if( j < 2 + (int) ( ( [bmp size].width - 4.0 ) * progress ) )
            {
                pen[j] = red;
            }
            else
            {
                pen[j] = white;
            }
        }
        pen[j]   = black;
        pen[j+1] = black;
    }
    for( i = row_end - 2; i < row_end; i++ )
    {
        pen = (uint32_t *) ( [bmp bitmapData] + i * [bmp bytesPerRow] );
        for( j = 0; j < (int) [bmp size].width; j++ )
        {
            pen[j] = black;
        }
    }
	
    /* Now update the dock icon */
    tiff = [bmp TIFFRepresentationUsingCompression:
					  NSTIFFCompressionNone factor: 1.0];
    icon = [[NSImage alloc] initWithData: tiff];
    [NSApp setApplicationIconImage: icon];
    [icon release];
}

- (id) updatePopUpIcon: (id) value
{
    if (!value)
        return nil;
    
    NSImage * icon;
    
    icon = [[NSWorkspace sharedWorkspace] iconForFile: value];
    
    [icon setScalesWhenResized: YES];
    [icon setSize: NSMakeSize(16.0 , 16.0)];
    
    return icon;
}

- (void) working: (NSNotification *) n
{
   float progress_total = ( p.working.progress + p.working.job_cur - 1 ) / p.working.job_count;
    NSMutableString * string = [NSMutableString stringWithFormat: @"Converting: %.1f %%", 100.0 * progress_total];
    
    if( p.working.seconds > -1 )
    {
        [string appendFormat: @" (%.1f fps, ", p.working.rate_avg];
        if( p.working.hours > 0 )
        {
        [string appendFormat: @"%d hour%s %d min%s",
            p.working.hours, p.working.hours == 1 ? "" : "s",
            p.working.minutes, p.working.minutes == 1 ? "" : "s"];
        }
        else if( p.working.minutes > 0 )
        {
            [string appendFormat: @"%d min%s %d sec%s",
                p.working.minutes, p.working.minutes == 1 ? "" : "s",
                p.working.seconds, p.working.seconds == 1 ? "" : "s"];
        }
        else
        {
            [string appendFormat: @"%d second%s",
                p.working.seconds, p.working.seconds == 1 ? "" : "s"];
        }
            [string appendString: @" left)"];
    }
    
    [fConvertInfoString setStringValue: string];
    [fConvertIndicator setIndeterminate: NO];
    [fConvertIndicator setDoubleValue: 100.0 * progress_total];
    [self UpdateDockIcon: progress_total];
}

- (void) muxing: (NSNotification *) n
{
    [fConvertInfoString setStringValue: NSLocalizedString(@"Muxing...",@"Muxing...")];
    [fConvertIndicator setIndeterminate: YES];
    [fConvertIndicator startAnimation: nil];
    [self UpdateDockIcon: 1.0];
}

- (void) workDone: (NSNotification *) n
{    
    [fConvertIndicator setIndeterminate: NO];
    [fConvertIndicator setDoubleValue: 0.0];
    [self UpdateDockIcon: -1.0];
    [self convertEnable: YES];
        
    [fConvertInfoString setStringValue: NSLocalizedString(@"Done.",@"Done.")];

    [fCore removeAllJobs];
}

@end
