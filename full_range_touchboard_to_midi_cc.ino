/*******************************************************************************

 Bare Conductive Touch and Proximity USB MIDI interface
 ------------------------------------------------------
 
 Midi_interface_generic.ino - USB MIDI touch and proximity example

 Allows the mapping of each electrode to a key or control modulator in a 
 (relatively) simple manner. See the comments for details and experiment
 for best results.
 
 Requires Arduino 1.5.6+ or greater and ARCore Hardware Cores 
 https://github.com/rkistner/arcore - remember to select 
 Bare Conductive Touch Board (arcore, iPad compatible) in the Tools -> Board menu
 
 Bare Conductive code written by Stefan Dzisiewski-Smith.
 
 This work is licensed under a Creative Commons Attribution-ShareAlike 3.0 
 Unported License (CC BY-SA 3.0) http://creativecommons.org/licenses/by-sa/3.0/
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.

*******************************************************************************/

#include <MPR121.h>
#include <Wire.h>
#include "Midi_object.h"
#include "Compiler_Errors.h"
MIDIEvent e;

#define numElectrodes 12
midi_object_t MIDIobjects[numElectrodes]; // create an array of MIDI objects to use (one for each electrode)

void setup() {
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);

  MPR121.begin(0x5C);
  MPR121.setInterruptPin(4);

 
  // set touch and release thresholds for electrodes that require it
  for(int i=0; i<numElectrodes; i++){
    // set up electrode 5 as a proxmity mapped controller attached to controller 102
    MIDIobjects[i].type = MIDI_CONTROL;
    MIDIobjects[i].controllerNumber = 100 + i; // 102..119 are undefined and so very useful for this
    MIDIobjects[i].inputMin = 150;  // minimum input from Touch Board - see Serial monitor for these values
                                  // to help in setting sensible min and max with / without hand in place
    MIDIobjects[i].inputMax = 700;  // maximum input from Touch Board
    MIDIobjects[i].outputMin = 0;   // minimum output to controller - smallest valid value is 0
    MIDIobjects[i].outputMax = 127; // minimum output to controller - largest valid value is 127
  }     

  // start with fresh data
  MPR121.updateAll();
}

void loop() {
  // check note electrodes
  if(MPR121.touchStatusChanged()){
    MPR121.updateTouchData();
    for(int i=0; i<numElectrodes; i++){
      if(MIDIobjects[i].type==MIDI_NOTE){ // if this is a note type object...
        e.type = 0x08;
        e.m2 = MIDIobjects[i].noteNumber; // set note number
        e.m3 = 127;  // maximum volume
        if(MPR121.isNewTouch(i)){
          // if we have a new touch, turn on the onboard LED and
          // send a "note on" message with the appropriate note set
          digitalWrite(LED_BUILTIN, HIGH);
          e.m1 = 0x90; 
        } else if(MPR121.isNewRelease(i)){
          // if we have a new release, turn off the onboard LED and
          // send a "note off" message
          digitalWrite(LED_BUILTIN, LOW);
          e.m1 = 0x80;
        } else {
          // else set a flag to do nothing...
          e.m1 = 0x00;  
        }
        // only send a USB MIDI message if we need to
        if(e.m1 != 0x00){
          MIDIUSB.write(e);
        }
      }
    }
  }

  MPR121.updateFilteredData();

  // now check controller electrodes
  for(int i=0; i<numElectrodes; i++){
    if(MIDIobjects[i].type==MIDI_CONTROL){ // if we have a control type object...
      Serial.print("E");
      Serial.print(i);
      Serial.print(":");                          // this prints some Serial debug data for ease of mapping
      Serial.println(MPR121.getFilteredData(i));  // e.g. E11:567 means E11 has value 567 (this is the input data)

      // output the correctly mapped value from the input
      e.m3 = (unsigned char)constrain(map(MPR121.getFilteredData(i), MIDIobjects[i].inputMin, MIDIobjects[i].inputMax, MIDIobjects[i].outputMin, MIDIobjects[i].outputMax), 0, 127);

      if(e.m3!=MIDIobjects[i].lastOutput){ // only output a new controller value if it has changed since last time

        MIDIobjects[i].lastOutput=e.m3;

        e.type = 0x08;
        e.m1 = 0xB0; // control change message
        e.m2 = MIDIobjects[i].controllerNumber;     // select the correct controller number - you should use numbers
                                                    // between 102 and 119 unless you know what you are doing
        MIDIUSB.write(e);
      }
    }
  }

  // flush USB buffer to ensure all notes are sent
  MIDIUSB.flush(); 

  delay(10); // 10ms delay to give the USB MIDI target time to catch up

}
