//
//  Preset.h
//  InstantHandBrake
//
//  Created by Damiano Galassi on 15/01/08.
//  This file is part of the HandBrake source code.
//  Homepage: <http://handbrake.fr/>.
//  It may be used under the terms of the GNU General Public License.
//
//

#import <Cocoa/Cocoa.h>
#import "Preset.h"


@interface Device : NSObject <NSCoding> {
    NSString            * deviceName;
    NSMutableArray      * presetsArray;
}

- (id) initWithDeviceName:(NSString *) name;
- (void) addPreset: (Preset *) preset;

- (NSString *) name;
- (Preset *) firstPreset;

@end
