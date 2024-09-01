# BatchCraze

This is a game I created that I'm calling Batch Craze, which plays the same as a popular game called Catch Phrase (a game that my family, friends, and I enjoy). A word of phrase is displayed on the screen, and you have to get the other players to guess it without saying the word or phrase.
We picked up the NSFW "After Dark" version and were disappointed with the glossary of “edgy” phrases. This inspired me to build my own, with the ability to update the phrase/word list easily. My version has a couple of added features:
- Touch screen instead of buttons
- Players can connect to the device's AP and submit suggestions to be added to the master list of phrases/words.
- Suggested items can then be reviewed via a web interface, and if approved will be added to the master list.

This project is built around the ESP32-2432S028 aka Cheap Yellow Display. The following GitHub page was very useful when creating this project, check it out! https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display

The project is pretty straight forward. The player presses anywhere on the screen to start the game, and a random word/phrase from the Master_List.txt file on the SD card is selected and shown on the screen. They have to get another player to guess the word, then pass the unit to the next player who does the same.

Once a word is randomly chosen and displayed, it is moved from the Master_List.txt file to the Used_Words.txt file. This ensures all words/phrases in the Master List are used precisely once, even in between game sessions. Once the Master List is empty, all phrases in the Used Words list are moved back into the Master List. The master list is currently about 1300 lines long (thanks ChatGPT). 

You can also connect to the Wifi Access Point "BatchCraze" that is broadcasted from the game. Once connected, go to http://192.168.4.1/ and submit new phrases or words to be added to the game! These suggestions are populated into the Submitted.txt file on the SD card. You can then connect and go to http://192.168.4.1/review/ and approve (individually, or all at once) or deny the suggestions. The approved items are added to the Master_List.txt file. I added the option to Deny because I foresee some of my friends adding ridiculous words/phrases that I may not want in the game.

Missing features still in development:

- Get the speaker working. I have a timer mp3 file that I'd like to play through the speaker to give a time limit, but haven't got the code working yet. 
