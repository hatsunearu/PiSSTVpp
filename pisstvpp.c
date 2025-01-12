// PiSSTVpp

// 2013 Robert Marshall KI4MCW
// 2014 Gerrit Polder, PA3BYA fixed header. 
// 2014 Don Gi Min, KM4EQR, more protocols and option handling

// Note: Compile me thus:  gcc -lgd -lmagic -o sstvX sstvX.c

// ===========
// includes
// ===========

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <gd.h>
#include <time.h>
#include <math.h>
#include <tgmath.h>
#include <magic.h>
#include <unistd.h>


// ================
// macros/defines
// ================

#define RATE   11025 
#define MAXRATE   22050
#define BITS   16
#define CHANS  1 
#define VOLPCT 20 
// ^-- 90% max
#define MAXSAMPLES (300 * MAXRATE)

// uncomment only one of these
#define AUDIO_WAV
//#define AUDIO_AIFF

#define MAGIC_PNG ("PNG image data,")
#define MAGIC_JPG ("JPEG image data")
#define MAGIC_CNT 15

#define FILETYPE_ERR 0
#define FILETYPE_PNG 1
#define FILETYPE_JPG 2

// =========
// globals
// =========

uint16_t   g_audio[MAXSAMPLES] ;
uint32_t   g_scale, g_samples ;
double     g_twopioverrate , g_uspersample ; 
double     g_theta, g_fudge ; 
uint8_t    g_protocol; //VIS ID's for SSTV protocols

FILE *     g_imgfp ;
FILE *     g_outfp ;
gdImagePtr g_imgp ;
uint16_t   g_rate;

// ========
// protos
// ========

uint8_t  filetype       (char *filename) ;
void     playtone       (uint16_t tonefreq , double tonedur) ;
void     addvisheader   (void) ;
void     addvistrailer  (void) ;

uint16_t toneval_rgb        (uint8_t colorval) ;
uint16_t toneval_yuv        (uint8_t colorval) ;

void     buildaudio_m     (double pixeltime) ;
void     buildaudio_s     (double pixeltime) ;
void     buildaudio_r36     (void) ;

#ifdef AUDIO_AIFF
void     writefile_aiff (void) ;
#endif
#ifdef AUDIO_WAV
void     writefile_wav  (void) ;
#endif


// ================
//   main
// ================

int main(int argc, char *argv[]) {

    char *protocol = NULL;
    int option;

    g_rate = RATE;
    while ((option = getopt(argc, argv, "r:p:")) != -1) {
        switch (option) {
            case 'r':
                g_rate = (atoi(optarg));
                break;
            case 'p':
                protocol = optarg;
                break;
        }
    }

    //Set VIS codes
    if ( !protocol ) {
        printf("No protocol option, defaulting to Martin 1...\n\n");
        g_protocol = 44;
    }
    else if ( strcmp(protocol, "m1") == 0 ) {
        g_protocol = 44; //Martin 1
    }
    else if ( strcmp(protocol,"m2") == 0) {
        g_protocol = 40; //Martin 2
    }
    else if ( strcmp(protocol,"s1") == 0) {
        g_protocol = 60; //Scottie 1
    }
    else if ( strcmp(protocol,"s2") == 0) {
        g_protocol = 56; //Scottie 2
    }
    else if ( strcmp(protocol,"sdx") == 0) {
        g_protocol = 76; //Scottie DX
    }
    else if ( strcmp(protocol,"r36") == 0) {
        g_protocol = 8; //Robot 36
    }
    else {
        printf("Unrecognized protocol option %s, defaulting to Martin 1...\n\n", protocol);
        g_protocol = 44;
    }


    // locals
    uint32_t starttime = time(NULL) ;
    uint8_t ft ; 
    char inputfile[255], outputfile[255] ;
    
    // string hygeine
    memset( inputfile  , 0 , 255 ) ;
    memset( outputfile , 0 , 255 ) ;
    
    // assign values to globals

    double temp1, temp2, temp3 ;
    temp1 = (double)( 1 << (BITS - 1) ) ;
    temp2 = VOLPCT / 100.0 ;
    temp3 = temp1 * temp2 ;
    g_scale = (uint32_t)temp3 ;
    
    g_twopioverrate = 2.0 * M_PI / g_rate ;
    g_uspersample = 1000000.0 / (double)g_rate ;

    g_theta = 0.0 ;
    g_samples = 0.0 ;
    g_fudge = 0.0 ;

    printf( "Constants check:\n" ) ;
    printf( "      rate = %d\n" , g_rate ) ;
    printf( "  VIS code = %d\n" , g_protocol);
    printf( "      BITS = %d\n" , BITS ) ;
    printf( "    VOLPCT = %d\n" , VOLPCT ) ; 
    printf( "     scale = %d\n" , g_scale ) ;
    printf( "   us/samp = %f\n" , g_uspersample ) ;
    printf( "   2p/rate = %f\n\n" , g_twopioverrate ) ;

    if ( optind >= argc ) {
        printf("No file arguments given.\n");
        return 2 ;
    }

    // set filenames    
    strncpy( inputfile , argv[optind] , strlen( argv[optind] ) ) ;
    ft = filetype( inputfile ) ;
    if ( ft == FILETYPE_ERR ) 
    {
        printf( "Exiting.\n" ) ;
        return 2 ;
    }
    
    strncpy( outputfile, inputfile , strlen( inputfile ) ) ;
    
#ifdef AUDIO_AIFF    
    strcat( outputfile , ".aiff" ) ;
#endif
#ifdef AUDIO_WAV    
    strcat( outputfile , ".wav" ) ;
#endif
    
    printf( "Input  file is [%s].\n" , inputfile ) ;
    printf( "Output file is [%s].\n" , outputfile ) ;
    
    // prep
    
    g_imgfp = fopen( inputfile , "r" ) ;
    g_outfp = fopen( outputfile , "w" ) ;
    printf( "FILE ptrs opened.\n" ) ;
    
    if ( ft == FILETYPE_JPG ) 
    { g_imgp = gdImageCreateFromJpeg( g_imgfp ) ; }
    else if ( ft == FILETYPE_PNG ) 
    { g_imgp = gdImageCreateFromPng( g_imgfp ) ; }
    else {
        printf( "Some weird error!\n" ) ;
        return 3 ;
    }    
    
    printf( "Image ptr opened.\n" ) ;

    // go!

    addvisheader() ;
    
    //Selects audio format mode
    switch (g_protocol) {
        case 44: //Martin 1
            buildaudio_m(457.6);
            break;
        case 40: //Martin 2
            buildaudio_m(228.8);
            break;
        case 60: //Scottie 1
            buildaudio_s(432.0);
            break;
        case 56: //Scottie 2
            buildaudio_s(275.2);
            break;
        case 76: //Scottie DX
            buildaudio_s(1080.0);
            break;
        case 8: //Robot 36
            buildaudio_r36();
            break;
        default:
            printf("Something went horribly wrong: unknown protocol while building audio!\n");
            exit(2);
            break;
    }


    addvistrailer() ;
    
#ifdef AUDIO_AIFF    
    writefile_aiff() ;
#endif
#ifdef AUDIO_WAV
    writefile_wav();
#endif    

    // cleanup

    fclose( g_imgfp ) ;
    fclose( g_outfp ) ;
    
    // brag
    
    uint32_t endtime = time(NULL) ;
    printf( "Created soundfile in %d seconds.\n" , ( endtime - starttime ) ) ;
    
    return 0 ;
}


// =====================
//  subs 
// =====================    


// filetype -- Check to see if input file is in one of our
//             supported formats (currently jus JPEG and PNG).
//             Uses libmagic.

uint8_t filetype( char *filename ) {
    magic_t m ;
    char m_str[ MAGIC_CNT + 2 ] ;
    uint8_t retval ;
    
    printf( "  Checking filetype for file [%s]\n" , filename ) ;
    
    retval = FILETYPE_ERR ;
    
    m = magic_open( MAGIC_NONE ) ;
    if ( m && ( magic_load(m, NULL) == 0 ) )
    {
        strncpy(m_str, magic_file(m, filename), MAGIC_CNT+1) ;
        
        if ( strncmp(m_str, MAGIC_JPG, MAGIC_CNT) == 0 )
        { 
            printf( "  File is a JPEG image.\n" ) ;
            retval = FILETYPE_JPG ; 
        }
        
        else if ( strncmp(m_str, MAGIC_PNG, MAGIC_CNT) == 0 )
        { 
            printf( "  File is a PNG image.\n" ) ;
            retval = FILETYPE_PNG ; 
        }
        
        else
        {
            printf( "  This file format is not supported!\n" ) ;
            printf( "  Please use a JPEG or PNG file instead.\n" ) ;
        }    
    }
    
    if ( m ) { magic_close(m) ; }
    
    return retval ;
}


// playtone -- Add waveform info to audio data. New waveform data is 
//             added in a phase-continuous manner according to the 
//             audio frequency and duration provided. Note that the
//             audio is still in a purely hypothetical state - the
//             format of the output file is not determined until 
//             the file is written, at the end of the process.
//             Also, yes, a nod to Tom Hanks.

void playtone( uint16_t tonefreq , double tonedur ) {
    uint16_t tonesamples, voltage, i ;
    double   deltatheta ;
     
    tonedur += g_fudge ;
    tonesamples = ( tonedur / g_uspersample ) + 0.5 ;
    deltatheta = g_twopioverrate * tonefreq ;
    
    for ( i=1 ; i<=tonesamples ; i++ ) {
        g_samples++ ;
        
        if ( tonefreq == 0 ) { g_audio[ g_samples ] = 32768 ; }
        else {

#ifdef AUDIO_AIFF        
            voltage = 32768 + (int)( sin( g_theta ) * g_scale ) ;
#endif
#ifdef AUDIO_WAV
            voltage =     0 + (int)( sin( g_theta ) * g_scale ) ;
#endif            
            g_audio[g_samples] = voltage ;
            g_theta += deltatheta ;
        }
    } // end for i        
    
    g_fudge = tonedur - ( tonesamples * g_uspersample ) ;
}  // end playtone    

uint16_t toneval_rgb ( uint8_t colorval ) {
    return ( ( 800 * colorval ) / 256 ) + 1500 ;
}

uint16_t toneval_yuv ( uint8_t colorval ) {
    return ( (float)colorval * 3.1372549 ) + 1500.0 ;
}    

// addvisheader -- Add the specific audio tones that make up the 
//                 Martin 1 VIS header to the audio data. Basically,
//                 this just means lots of calls to playtone(). 

void addvisheader() {
    printf( "Adding VIS header to audio data.\n" ) ;
    
    // bit of silence
    playtone(    0 , 500000 ) ;   

    // attention tones
    playtone( 1900 , 100000 ) ; // you forgot this one
    playtone( 1500 , 100000 ) ;
    playtone( 1900 , 100000 ) ;
    playtone( 1500 , 100000 ) ;
    playtone( 2300 , 100000 ) ;
    playtone( 1500 , 100000 ) ;
    playtone( 2300 , 100000 ) ;
    playtone( 1500 , 100000 ) ;
                    
    // VIS lead, break, mid, start
    playtone( 1900 , 300000 ) ;
    playtone( 1200 ,  10000 ) ;
    playtone( 1900 , 300000 ) ;
    playtone( 1200 ,  30000 ) ;

    int i;
    int parity = 0;

    //write VIS data tones
    for(i = 1; i <= 64; i = i<<1) {
        if(i & g_protocol) { //digit is a 1
            playtone(1100, 30000);
            parity = !parity;
        }
        else {
            playtone(1300, 30000); //digit is a 0
        }
    }

    //parity bit
    if(parity) //odd parity
        playtone(1100, 30000);
    else       //even parity
        playtone(1300, 30000);

    // VIS stop
    playtone( 1200 ,  30000 ) ; 
    printf( "Done adding VIS header to audio data.\n" ) ;
        
} // end addvisheader   

//Builds audio scan data for the Martin series of specifications.
//Applicable to Martin 1 and Martin 2 SSTV modes
//Each pixel is scanned for pixeltime microseconds
void buildaudio_m (double pixeltime) {
    uint16_t x , y , k ;
    uint32_t pixel ;
    uint8_t r[320], g[320], b[320] ;
    
    printf( "Adding image to audio data.\n" ) ;
    
    for ( y=0 ; y<256 ; y++ ) {
    
        // read image data
        for ( x=0 ; x<320 ; x++ ) {
            pixel = gdImageGetTrueColorPixel( g_imgp, x, y ) ;
            //printf( "Got pixel.\n" ) ;
            
            // get color data
            r[x] = gdTrueColorGetRed( pixel ) ;
            g[x] = gdTrueColorGetGreen( pixel ) ;
            b[x] = gdTrueColorGetBlue( pixel ) ;
        }
        
        // add row markers to audio
        // sync
        playtone( 1200 , 4862 ) ;
        // porch 
        playtone( 1500 ,  572 ) ;
        
        // each pixel is 457.6us long in Martin 1
        
        // add audio for green channel for this row
        for ( k=0 ; k<320 ; k++ )
        { playtone( toneval_rgb( g[k] ) , pixeltime ) ; }
        
        // separator tone 
        playtone( 1500 , 572 ) ;
        
        // bloo channel
        for ( k=0 ; k<320 ; k++ )
        { playtone( toneval_rgb( b[k] ) , pixeltime ) ; }

        playtone( 1500 , 572 ) ;

        // red channel
        for ( k=0 ; k<320 ; k++ )
        { playtone( toneval_rgb( r[k] ) , pixeltime ) ; }

        playtone( 1500 , 572 ) ;
        
    }  // end for y
    
    printf( "Done adding image to audio data.\n" ) ;    
    
}  // end buildaudio_m   

//Builds audio scan data for the Scottie series of specifications.
//Applicable to Scottie 1, Scottie 2, Scottie  SSTV modes
//Each pixel is scanned for pixeltime microseconds
void buildaudio_s (double pixeltime) {
    uint16_t x , y , k ;
    uint32_t pixel ;
    uint8_t r[320], g[320], b[320] ;
    
    printf( "Adding image to audio data.\n" ) ;
    
    //add starting sync pulse
    playtone( 1200 , 9000);

    for ( y=0 ; y<256 ; y++ ) {
        // read image data
        for ( x=0 ; x<320 ; x++ ) {
            pixel = gdImageGetTrueColorPixel( g_imgp, x, y ) ;
            
            // get color data
            r[x] = gdTrueColorGetRed( pixel ) ;
            g[x] = gdTrueColorGetGreen( pixel ) ;
            b[x] = gdTrueColorGetBlue( pixel ) ;
        }
        //seperator pulse
        playtone(1500, 1500);
        
        // add audio for green channel for this row
        for ( k=0 ; k<320 ; k++ )
            playtone( toneval_rgb( g[k] ) , pixeltime ) ;

        // separator tone 
        playtone(1500, 1500) ;

        // blue channel
        for ( k=0 ; k<320 ; k++ )
            playtone( toneval_rgb( b[k] ) , pixeltime ) ; 
     

        //sync pulse
        playtone(1200, 9000);

        //sync porch
        playtone(1500 , 1500) ;

        // red channel
        for ( k=0 ; k<320 ; k++ )
            playtone( toneval_rgb( r[k] ) , pixeltime ) ;

        
    }  // end for y
    
    printf( "Done adding image to audio data.\n" ) ;    
    
}  // end buildaudio_s


//Builds audio scan data for the Robot 36.
//Applicable to only Robot 36.
void buildaudio_r36 () {

    uint16_t x , y , k ;
    uint32_t pixel1, pixel2;
    uint8_t r1, g1, b1, r2, g2, b2, avgr, avgg, avgb;
    uint8_t y1[320], y2[320], ry[320], by[320];

    printf( "Adding image to audio data.\n" ) ;
    
    for ( y=0 ; y<240 ; y+=2 ) {
    
        // read image data
        for ( x=0 ; x<320 ; x++ ) {

            //even line pixel
            pixel1 = gdImageGetTrueColorPixel( g_imgp, x, y ) ;
            //odd line pixel 
            pixel2 = gdImageGetTrueColorPixel( g_imgp, x, y+1) ;
            
            // get color data
            r1 = gdTrueColorGetRed( pixel1 ); //first line (even) of red intensities
            r2 = gdTrueColorGetRed( pixel2 ); //second line (odd) of red intensities
            g1 = gdTrueColorGetGreen( pixel1 );
            g2 = gdTrueColorGetGreen( pixel2 );
            b1 = gdTrueColorGetBlue( pixel1 );
            b2 = gdTrueColorGetBlue( pixel2 );

            avgr = (uint8_t)( ((uint16_t)r1 + (uint16_t)r2) / 2 );

            avgg = (uint8_t)( ((uint16_t)g1 + (uint16_t)g2) / 2 );

            avgb = (uint8_t)( ((uint16_t)b1 + (uint16_t)b2) / 2 );

            //Y value of even lines 
            y1[x] = 16.0 + (0.003906 * ((65.738 * (float)r1) + (129.057 * (float)g1) + (25.064 * (float)b1)));
            //Y value of odd lines
            y2[x] = 16.0 + (0.003906 * ((65.738 * (float)r2) + (129.057 * (float)g2) + (25.064 * (float)b2)));
            //R-Y value of the average of the two lines, to be transmitted even scans
            ry[x] = 128.0 + (0.003906 * ((112.439 * (float)avgr) + (-94.154 * (float)avgg) + (-18.285 * (float)avgb)));
            //B-Y value of the average of the two lines, to be transmitted odd scans
            by[x] = 128.0 + (0.003906 * ((-37.945 * (float)avgr) + (-74.494 * (float)avgg) + (112.439 * (float)avgb)));

        }
        
        //begin robot 36 code

        //even lines    
        //sync
        playtone( 1200 , 9000 ) ;
        //porch 
        playtone( 1500 , 3000 ) ;
        
        //y scan, even, 88ms total, 320 points, 275us per pixel
        for ( k = 0; k < 320; k++ ) {
            playtone( toneval_yuv( y1[k] ) , 275 ) ;
        }
        
        //even line seperator
        playtone( 1500 , 4500 );
        //porch
        playtone( 1900 , 1500 );

        //R-Y scan, 44ms total, 320 points, 137.5us per pixel
        for ( k = 0; k < 320; k++ ) {
            playtone( toneval_yuv( ry[k] ) , 137.5 );
        }

        //odd lines
        // sync
        playtone( 1200 , 9000 ) ;
        // porch 
        playtone( 1500 , 3000 ) ;

        //y scan, odd, 88ms total, 320 points, 275us per pixel
        for ( k = 0; k < 320; k++ ) {
                playtone( toneval_yuv( y2[k] ) , 275 ) ;
        }

        //odd line seperator
        playtone( 2300 , 4500 );
        //porch
        playtone( 1900 , 1500 );

        //B-Y scan, 44ms total, 320 points, 137.5us per pixel
        for ( k = 0; k < 320; k++) {
            playtone( toneval_yuv( by[k] ) , 137.5);
        }
  
    }  // end for y
    
    printf( "Done adding image to audio data.\n" ) ;    
    
}  // end buildaudio_r36

// addvistrailer -- Add tones for VIS trailer to audio stream.
//                  More calls to playtone(). 

void addvistrailer () {
    printf( "Adding VIS trailer to audio data.\n" ) ;
    
    playtone( 2300 , 300000 ) ;
    playtone( 1200 ,  10000 ) ;
    playtone( 2300 , 100000 ) ;
    playtone( 1200 ,  30000 ) ;
    
    // bit of silence
    playtone(    0 , 500000 ) ;
    
    printf( "Done adding VIS trailer to audio data.\n" ) ;    
}

// writefile_aiff -- Save audio data to an AIFF file. Playback for
//                   AIFF format files is tricky. This worked on 
//                   ARM Linux:
//                     aplay -r 11025 -c 1 -f U16_BE file.aiff
//                   The WAV output is much easier and more portable, 
//                   but who knows - this code might be useful for 
//                   something. 

#ifdef AUDIO_AIFF
void writefile_aiff () {
    uint32_t totalsize , audiosize , i ;
    audiosize = 8 + ( 2 * g_samples ) ;      // header + 2bytes/samp
    totalsize = 4 + 8 + 18 + 8 + audiosize ;

    printf( "Writing audio data to file.\n" ) ;
    printf( "Got a total of [%d] samples.\n" , g_samples ) ;
    
    // "form" chunk
    fputs( "FORM" , g_outfp ) ;
    fputc( (totalsize & 0xff000000) >> 24 , g_outfp ) ;
    fputc( (totalsize & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (totalsize & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (totalsize & 0x000000ff)       , g_outfp ) ;    
    fputs( "AIFF" , g_outfp ) ;
    
    // "common" chunk
    fputs( "COMM" , g_outfp ) ;
    fputc(    0 , g_outfp ) ;   // size
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(   18 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;   // channels = 1
    fputc(    1 , g_outfp ) ;
    fputc( (g_samples & 0xff000000) >> 24 , g_outfp ) ;   // size
    fputc( (g_samples & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (g_samples & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (g_samples & 0x000000ff)       , g_outfp ) ;    
    fputc(    0 , g_outfp ) ;  // bits/sample
    fputc(   16 , g_outfp ) ;
    fputc( 0x40 , g_outfp ) ;  // 10 byte sample rate (??)
    fputc( 0x0c , g_outfp ) ;  // <--- 11025
    fputc( 0xac , g_outfp ) ;
    fputc( 0x44 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    
    // audio data chunk
    fputs( "SSND" , g_outfp ) ;
    fputc( (audiosize & 0xff000000) >> 24 , g_outfp ) ;
    fputc( (audiosize & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (audiosize & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (audiosize & 0x000000ff)       , g_outfp ) ;    
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    
    // FINALLY, the audio data itself
    for ( i=0 ; i<=g_samples ; i++ )
    {
        fputc( ( g_audio[i] & 0xff00 ) >> 8 , g_outfp ) ;
        fputc( ( g_audio[i] & 0x00ff )      , g_outfp ) ;
    }
    
    printf( "Done writing to audio file.\n" ) ;
}
#endif 

// writefile_wav -- Write audio data to a WAV file. Once the file
//                  is written, any audio player in the world ought
//                  to be able to play the file without any funky
//                  command-line params.

#ifdef AUDIO_WAV
void writefile_wav () {
    uint32_t totalsize , audiosize , byterate , blockalign ;
    uint32_t i ;
    
    audiosize  = g_samples * CHANS * (BITS / 8) ; // bytes of audio
    totalsize  = 4 + (8 + 16) + (8 + audiosize) ; // audio + some headers
    byterate   = g_rate * CHANS * BITS / 8 ;        // audio bytes / sec
    blockalign = CHANS * BITS / 8 ;               // total bytes / sample
    
    printf( "Writing audio data to file.\n" ) ;
    printf( "Got a total of [%d] samples.\n" , g_samples ) ;
    
    // RIFF header 
    fputs( "RIFF" , g_outfp ) ;
    
    // total size, audio plus some headers (LE!!)
    fputc( (totalsize & 0x000000ff)       , g_outfp ) ;    
    fputc( (totalsize & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (totalsize & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (totalsize & 0xff000000) >> 24 , g_outfp ) ;
    fputs( "WAVE" , g_outfp ) ;
    
    // sub chunk 1 (format spec)
    
    fputs( "fmt " , g_outfp ) ;  // with a space!
    
    fputc(   16 , g_outfp ) ;   // size of chunk (LE!!)
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    fputc(    0 , g_outfp ) ;
    
    fputc(    1 , g_outfp ) ;   // format = 1 (PCM) (LE)
    fputc(    0 , g_outfp ) ;
    
    fputc(    1 , g_outfp ) ;   // channels = 1 (LE)
    fputc(    0 , g_outfp ) ;
    
    // samples / channel / sec (LE!!)
    fputc( (g_rate & 0x000000ff)       , g_outfp ) ;
    fputc( (g_rate & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (g_rate & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (g_rate & 0xff000000) >> 24 , g_outfp ) ;

    // bytes total / sec (LE!!)
    fputc( (byterate & 0x000000ff)       , g_outfp ) ;    
    fputc( (byterate & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (byterate & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (byterate & 0xff000000) >> 24 , g_outfp ) ;

    // block alignment (LE!!)
    fputc( (blockalign & 0x00ff)       , g_outfp ) ;    
    fputc( (blockalign & 0xff00) >>  8 , g_outfp ) ;    
    
    fputc( (BITS & 0x00ff)       , g_outfp ) ;   // bits/sample (LE)
    fputc( (BITS & 0xff00) >>  8 , g_outfp ) ; 

    // sub chunk 2
    // header
    fputs( "data" , g_outfp ) ;
    
    // audio bytes total (LE!!)
    fputc( (audiosize & 0x000000ff)       , g_outfp ) ;    
    fputc( (audiosize & 0x0000ff00) >>  8 , g_outfp ) ;    
    fputc( (audiosize & 0x00ff0000) >> 16 , g_outfp ) ;    
    fputc( (audiosize & 0xff000000) >> 24 , g_outfp ) ;
    
    // FINALLY, the audio data itself (LE!!)
    for ( i=0 ; i<=g_samples ; i++ ) {
        fputc( ( g_audio[i] & 0x00ff )      , g_outfp ) ;
        fputc( ( g_audio[i] & 0xff00 ) >> 8 , g_outfp ) ;
    }

    // no trailer    
    printf( "Done writing to audio file.\n" ) ;
}
#endif

        
// end
