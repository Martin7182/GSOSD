Howto generate a GSOSD font file
================================

Many (Micro-) Minim OSD's come with a useless font. Therefore it is important to generate and upload a fresh font definition first. GSOSD uses the normal ascii-set, that consists of printable characters in the ranges 32 - 126 and 160 - 255. Character ranges 0 - 31 and 128 - 159 are reserved for special purposes; currently used by GSOSD:  
0 			: indicates empty screen positions  
1 - 8 			: defines window borders  
128 - 139, 144 - 155 	: defines the startup logo  
140 - 143, 156 - 159 	: defines alternative window borders  

A new font file is simply a GSOSD script consisting of 256 SET_FONT instructions. You can type this in by hand, but much more convenient is to use gif2osd.php that converts a 192x288 pixel gif-image in one go. The image should contain 16x16 characters, each of Max7456's character image size which is 12x18 pixels. Creating such image is easier than it looks. I just used Gimp for Ubuntu Linux as follows:  
- Use script iso-8859-1.php or any other tool to generate a 16x16 text block that shows all characters. You can also copy/paste this block, between START- and STOP-lines (use Western, ISO-8859-1 encoding):

        === START ===  
            
            
         !"#$%&'()*+,-./  
        0123456789:;<=>?  
        @ABCDEFGHIJKLMNO  
        PQRSTUVWXYZ[\]^_  
        `abcdefghijklmno  
        pqrstuvwxyz{|}~  
            
            
         ¡¢£¤¥¦§¨©ª«¬­®¯  
        °±²³´µ¶·¸¹º»¼½¾¿  
        ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏ  
        ÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞß  
        àáâãäåæçèéêëìíîï  
        ðñòóôõö÷øùúûüýþÿ  
        === STOP ===  

- Copy/paste this block into a text field of the image editor.  
- Select a fixed-width font of your choice.  
- Adjust the font size so that the image fits exactly in 192x288 pixels. In Gimp you can set the grid to 12x18 pixels to make this easier.  
- The background should be white and the characters should be black.  
- Optionally, define window borders and logo (see above).  
- Save the image in native format (so that you can edit it later if needed).  
- Save the image in gif layout.  
- Run command "gif2osd.php \<gif-file\> 2". The output is plain text that can be processed directly by GSOSD.  
- If you want to hard-code this font in the program sources, use a "3" instead of "2" as second parameter for gifosd.php. And copy/paste the output into src/font.cpp. Note that this defines white character pixels only. When resetting the font using command FONT_RESET, the 'black border' effect is applied by default. Other effects can be activated using command FONT_EFFECT.  


Font effect examples
====================

Border (default at reset)
------------------------
![abc_border.gif](../images/abc_border.gif)

Shadow
------
![abc_shadow.gif](../images/abc_shadow.gif)

Trans/white
-----------------
![abc_transwhite.gif](../images/abc_transwhite.gif)

Black/white
-----------
![abc_blackwhite.gif](../images/abc_blackwhite.gif)

Border + invert
--------------
![abc_border_invert.gif](../images/abc_border_invert.gif)

Shadow + invert
---------------
![abc_shadow_invert.gif](../images/abc_shadow_invert.gif)

Trans/white + invert
--------------------------
![abc_transwhite_invert.gif](../images/abc_transwhite_invert.gif)

Black/white + invert
--------------------
![abc_blackwhite_invert.gif](../images/abc_blackwhite_invert.gif)

Trans/white + invert + trans/white
----------------------------------------------
![abc_transwhite_invert_transwhite.gif](../images/abc_transwhite_invert_transwhite.gif)
