// StyleSheet_as.hx:  ActionScript 3 "StyleSheet" class, for Gnash.
//
// Generated by gen-as3.sh on: 20090515 by "rob". Remove this
// after any hand editing loosing changes.
//
//   Copyright (C) 2009, 2010 Free Software Foundation, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

// This test case must be processed by CPP before compiling to include the
//  DejaGnu.hx header file for the testing framework support.

#if flash9
import flash.text.StyleSheet;
import flash.display.MovieClip;
#else
import flash.MovieClip;
#end
import flash.Lib;
import Type;
import Std;

// import our testing API
import DejaGnu;

// Class must be named with the _as suffix, as that's the same name as the file.
class StyleSheet_as {
    static function main() {
        
#if flash9
        var x1:StyleSheet = new StyleSheet();

        // Make sure we actually get a valid class        
        if (x1 != null) {
            DejaGnu.pass("StyleSheet class exists");
        } else {
            DejaGnu.fail("StyleSheet class doesn't exist");
        }
	// Tests to see if all the properties exist. All these do is test for
	// existance of a property, and don't test the functionality at all. This
	// is primarily useful only to test completeness of the API implementation.
	if (Std.is(x1.styleNames, Dynamic)) {
	    DejaGnu.pass("StyleSheet.styleNames property exists");
	} else {
	    DejaGnu.fail("StyleSheet.styleNames property doesn't exist");
	}

	// Tests to see if all the methods exist. All these do is test for
	// existance of a method, and don't test the functionality at all. This
	// is primarily useful only to test completeness of the API implementation.
	if (Type.typeof(x1.getStyle)==ValueType.TFunction) {
	    DejaGnu.pass("StyleSheet::getStyle() method exists");
	} else {
	    DejaGnu.fail("StyleSheet::getStyle() method doesn't exist");
	}
	if (Type.typeof(x1.setStyle)==ValueType.TFunction) {
	    DejaGnu.pass("StyleSheet::setStyle() method exists");
	} else {
	    DejaGnu.fail("StyleSheet::setStyle() method doesn't exist");
	}
	if (Type.typeof(x1.clear)==ValueType.TFunction) {
	    DejaGnu.pass("StyleSheet::clear() method exists");
	} else {
	    DejaGnu.fail("StyleSheet::clear() method doesn't exist");
	}
	
	if (Type.typeof(x1.parseCSS)==ValueType.TFunction) {
	    DejaGnu.pass("StyleSheet::parseCSS() method exists");
	} else {
	    DejaGnu.fail("StyleSheet::parseCSS() method doesn't exist");
	}
	
	if (Type.typeof(x1.transform)==ValueType.TFunction) {
	    DejaGnu.pass("StyleSheet::transform() method exists");
	} else {
	    DejaGnu.fail("StyleSheet::transform() method doesn't exist");
	    DejaGnu.note("transform: " + Type.typeof(x1.transform));
	}


#else
    DejaGnu.note("StyleSheet did not exist before SWF9");
#end

        // Call this after finishing all tests. It prints out the totals.
        DejaGnu.done();
    }
}

// local Variables:
// mode: C++
// indent-tabs-mode: t
// End:

