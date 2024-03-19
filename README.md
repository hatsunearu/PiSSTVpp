# PiSSTVpp

PiSSTVpp is a simple implementation of various SSTV protocols, to be used by low-frills computers running a CLI, such as with Raspberry Pis and so on.
Its intended for use by ham radio amateurs.

## Compiling

To compile PiSSTVpp, you should download this repository and also have:

* gcc
* libgd
* libmagic
* make

On Ubuntu or Debian, you should be able to install those dependencies by typing into your terminal:

`sudo apt-get install build-essential libgd-dev libmagic-dev`

After that, type `make pisstvpp` in the directory where PiSSTV resides in.

## Usage

`./pisstvpp [-rSAMPLERATE] [-pPROTOCOL] imagename`

This command creates a WAVE file in the current working directory that encodes a PNG or JPG image into an SSTV audio signal. 

Supported protocols and their respective options:

* Martin 1: m1
* Martin 2: m2
* Scottie 1: s1
* Scottie 2: s2
* Scottie DX: sdx
* Robot 36: r36

`file` must be a PNG or JPG formatted image. Be advised that all modes except Robot 36 can only accept 320x256 sized images without cropping, whereas Robot 36 can only accept 320x240 sized images without cropping.

For instance, to create an audio file with the Scottie 2 protocol with a sampling rate of 22050Hz, and the input as a file `buzzece_qr.png` invoke:

`./pisstv -r 22050 -p s2 buzzece_qr.png`

This will create `buzzece_qr.png.wav` in the working directory that encodes the ECE Buzz image in the Scottie 2 protocol.

## Other things

There are some shell and Python scripts that carried over from the original repository by AgriVision (Gerrit Polder, PA3BYA). They should have been modified to work with the PiSSTVpp, but is not tested.

raspistill: to grab the image from the camera.
pisstv: to convert the image to a soundfile.
pifm_sstv: to transmit the soundfile over the air, e.g. on 144.5 MHz

pisstv is heavilly based on work from KI4MCW, which can be found here: https://sites.google.com/site/ki4mcw/Home/sstv-via-uc
I fixed some errors and made it a little bit more flexible.

pifm_sstv is based on the work of Oliver Mattos and Oskar Weigl  (http://www.icrobotics.co.uk/wiki/index.php/Turning_the_Raspberry_Pi_Into_an_FM_Transmitter).

The original program was intended for transmitting broadband stereo signals.
[AgriVision] adapted it a little bit so that the bandwidth can be set, which is very important for narrow-band ham radio transmissions. Also the timing can be tuned from the command-line, which is important for SSTV, where impropper timing results in slanted images.

sstvcatch is kind of a sstv security camera. A python script runs an endless loop, waits for image change, then transmits image data on 144.5 MHz using SSTV.

## License
All of the code contained here is licensed by the GNU General Public License v3.
A copy of the GPLv3 has been included in the repository.

## Credits

AgriVision (Gerrit Polder)

KI4MCW for SSTV

Oliver Mattos and Oskar Weigl for PiFM
