# doorbell
Ring detector for doorbell with Telegram and Web

- Description: sends a message to telegram when doorbell rings and also provides ring list via web

 - HW
    - If connecting to a 220v doorbell just use optocoupler module like this https://aliexpress.com/item/32809745991.htm to detect ringing without frying anything.
    - If you want to DIY, it is not that hard, an optocoupler and several resistor will do the work [https://circuitdigest.com/tutorial/opto-coupler-types-working-applications](https://circuitdigest.com/tutorial/opto-coupler-types-working-applications). ![AC sensor](https://circuitdigest.com/sites/default/files/inlineimages/u/Optocoupler-for-Detecting-AC-Voltage.png)
    
    - Connect signal from optocoupler to D6 (or any other pin you like), 3V3 and GND (I used Wemos D1 mini, others should work too)
    
 - SW
    -Upload data directory with [ESP8266SketchDataUpload](https://randomnerdtutorials.com/install-esp8266-filesystem-uploader-arduino-ide/) and flash it with USB for the first time. Then, when installed you can use OTA updates (especially useful if your doorbell is far from your PC).                           
              
