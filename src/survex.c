/* Display startup banner */
static void
display_banner(void)
{
   int msgGreet = /*Hello*/112;
#ifndef NO_NICEHELLO
   if (tmUserStart != (time_t)-1) {
      int hr;
      static uchar greetings[24] = {
         /*Good morning*/109,109,109,109, 109,109,109,109, 109,109,109,109,
         /*Good afternoon*/110,110,110,110, 110,110,/*Good evening*/111,111, 111,111,112,112
      };
      hr = (localtime(&tmUserStart))->tm_hour;
      msgGreet = (int)greetings[hr];
   }
#endif
   fputs(msg(msgGreet), stdout);
   puts(msg(/*, welcome to...*/113);
#ifndef NO_NICEHELLO
   puts("  ______   __   __   ______    __   __   _______  ___   ___");
   puts(" / ____|| | || | || |  __ \\\\  | || | || |  ____|| \\ \\\\ / //");
   puts("( ((___   | || | || | ||_) )) | || | || | ||__     \\ \\/ //");
   puts(" \\___ \\\\  | || | || |  _  //   \\ \\/ //  |  __||     >  <<");
   puts(" ____) )) | ||_| || | ||\\ \\\\    \\  //   | ||____   / /\\ \\\\");
   puts("|_____//   \\____//  |_|| \\_||    \\//    |______|| /_// \\_\\\\");
#else
   puts("SURVEX");
#endif
   putnl();
   puts(msg(/*Version*/114));
   puts(" "VERSION"\n"COPYRIGHT_MSG);
}

