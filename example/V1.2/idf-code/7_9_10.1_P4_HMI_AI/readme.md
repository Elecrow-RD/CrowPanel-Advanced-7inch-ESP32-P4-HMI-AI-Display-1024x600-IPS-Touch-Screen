## P4 HMI AI Demo User Guide

#### Preparation tools:

P4 HMI display (different screens correspond to different code, except for the code, all other operation steps are the same)

<img width="554" height="156" alt="image" src="https://github.com/user-attachments/assets/1594c6a3-462b-4892-93f4-745ae8e234fa" />

#### USB cable:

<img width="491" height="268" alt="image" src="https://github.com/user-attachments/assets/717313d7-c3de-46f4-9459-9e399fa74d2c" />

#### VS Code (ESP-IDF v5.4.2 installed)

<img width="553" height="300" alt="image" src="https://github.com/user-attachments/assets/6fd277fc-eeb0-4e1d-b233-646f7c8b28e2" />

Download the code for the corresponding screen size:

CrowPanel Advanced 5inch | ESP32-P4 HMI AI Display:


CrowPanel Advanced 7inch | ESP32-P4 HMI AI Display:
https://drive.google.com/file/d/1mGSaZ-9NiI8-cwcbhZgRR8-os7AhnqYb/view?usp=sharing

CrowPanel Advanced 9inch | ESP32-P4 HMI AI Display:
https://drive.google.com/file/d/1mGSaZ-9NiI8-cwcbhZgRR8-os7AhnqYb/view?usp=sharing

CrowPanel Advanced 10.1inch | ESP32-P4 HMI AI Display:
https://drive.google.com/file/d/1mGSaZ-9NiI8-cwcbhZgRR8-os7AhnqYb/view?usp=sharing

Use VS Code to open the project (here we take the 9-inch screen as an example, the operation steps are the same for other sizes):


##### 1. Open VS Code and choose to open the project folder.


<img width="554" height="301" alt="image" src="https://github.com/user-attachments/assets/c541d371-87ee-4bce-87d2-5a52202d18ba" />

##### 2. Select the folder you want to open, click “Select Folder” to open the project. Be careful not to double-click to open it.


<img width="521" height="289" alt="image" src="https://github.com/user-attachments/assets/f57169f3-a2be-4dc1-abb2-c38b013fc7d9" />


##### 3. Connect the device to the computer. Connect the device side to the UART0 port and plug it into the computer’s COM port.


<img width="553" height="325" alt="image" src="https://github.com/user-attachments/assets/383f0f38-c855-4f7b-a250-7bfbb1173ec4" />

##### 4. Before compiling, we need to modify the Wi-Fi account and password in the code to the ones that can currently connect (choose a 2.4G network): find main -> boards -> elecrow-p4-board -> elecrow_board.cc, and in lines 334–350, change all “elecrow888” to the Wi-Fi name you want to connect to. Change all “elecrow2014” to the Wi-Fi password you want to use.


<img width="554" height="354" alt="image" src="https://github.com/user-attachments/assets/7429e6d3-5a4c-47e3-875b-1f7b61cafc7e" />

Assume the Wi-Fi name you need to change to is: wifiP4, and the password is: 12345678, then modify it as follows:


<img width="554" height="426" alt="image" src="https://github.com/user-attachments/assets/1ecc9533-3a8f-46f8-8ee4-7c50cdc1ae68" />

##### 5. Select ESP-IDF v5.4.2.

<img width="554" height="458" alt="image" src="https://github.com/user-attachments/assets/b032555b-89c5-4fec-8280-24b257672299" />

##### 6. Select the flashing method.

<img width="481" height="401" alt="image" src="https://github.com/user-attachments/assets/261c86f2-3ece-42cb-8a7b-a11a56ec954c" />

##### 7. Select the COM port.

<img width="553" height="457" alt="image" src="https://github.com/user-attachments/assets/89c7ac86-2f07-4579-9e41-0a51c63d711f" />

##### 8. Select the flashing development board – esp32p4.

<img width="554" height="456" alt="image" src="https://github.com/user-attachments/assets/78c044f8-5def-4c4b-bebe-3d107e5ec9ea" />

##### 9. Before flashing, you can choose to clear the previous “build” cache. This step is not necessary, but you can choose to clear it. If you have modified the code, clearing the previous cache will improve the compilation success rate.

<img width="382" height="379" alt="image" src="https://github.com/user-attachments/assets/c6e7a284-1d00-4c9a-9ce2-1d5a46ef871a" />

##### 10. Compile the project. Please make sure the network connection is good. The compilation process may take some time, so please wait patiently.

<img width="554" height="562" alt="image" src="https://github.com/user-attachments/assets/6200c54a-fb5f-4853-ba17-44546b517107" />

This means the compilation is successful:

<img width="554" height="251" alt="image" src="https://github.com/user-attachments/assets/4bb2d4ea-4d95-4fc5-9576-8332185ce3cc" />

##### 11. Click to flash to the P4 HMI development board.

<img width="554" height="342" alt="image" src="https://github.com/user-attachments/assets/b2789f91-9036-46d0-957e-20ae4b7611d4" />

##### 12. After flashing is completed, when the device connects to Wi-Fi, the screen will display the verification code for connecting the device:


<img width="554" height="368" alt="image" src="https://github.com/user-attachments/assets/24e83933-94dd-4333-a4a8-c30cfa82e50e" />

##### 13. On the computer, log in to the “xiaozhi.me” website to configure the device, and first set it to the English version.

<img width="553" height="142" alt="image" src="https://github.com/user-attachments/assets/8014ec84-e355-436d-86c9-88f1c1f14c0a" />

##### 14. Click “Console” to enter the console.

<img width="554" height="215" alt="image" src="https://github.com/user-attachments/assets/eedd1c0a-ee6a-4a9b-8f07-2702cc0c57a9" />

##### 15. Click “Add Device” to add a device.

<img width="554" height="130" alt="image" src="https://github.com/user-attachments/assets/525b3fa8-8c44-41f9-8bd1-f22fd3ba8b7a" />

##### 16. Enter the device code, then click “Confirm”. Note that the device code here must be based on the output displayed on your own device.


<img width="554" height="162" alt="image" src="https://github.com/user-attachments/assets/143bf782-02f9-47c8-b680-6cd165cf39e1" />

##### 17. Select “Open Source” and click “Start Using”.

<img width="540" height="558" alt="image" src="https://github.com/user-attachments/assets/246c3b09-b600-436d-a5c9-beeab07a33a0" />

##### 18. Now your AI device can already chat. Since the default language is Chinese, we also need to configure the role. Click “Configure Role”.

<img width="554" height="336" alt="image" src="https://github.com/user-attachments/assets/0bd027f9-c66e-4d36-811b-18ced138f4ea" />

##### 19. Set “Dialogue Language” to “English”.

<img width="554" height="291" alt="image" src="https://github.com/user-attachments/assets/2fb62701-3251-493b-98d5-560dda0ff7e4" />

##### 20. Save the configuration.

<img width="554" height="485" alt="image" src="https://github.com/user-attachments/assets/14cc7bce-309a-4f1f-baab-d0aecfb44cff" />

##### 21. Next, we set the wake word. First, go back to “Agents”.

<img width="554" height="268" alt="image" src="https://github.com/user-attachments/assets/0dd57b38-4b71-44bb-9eb0-d0963c18a223" />

##### 22. Select “Manage Devices”.

<img width="554" height="222" alt="image" src="https://github.com/user-attachments/assets/4f09835b-47b9-4648-9ca1-951e47b88011" />

##### 23. Select “Customize”, then choose “English”, and click “Next” to enter the next configuration step.

<img width="554" height="288" alt="image" src="https://github.com/user-attachments/assets/494f483a-1b68-408a-bbf4-080f3fdc70ba" />

##### 24. In “Select Wake Word”, choose your wake word, or you can select “No Wake Word” if you do not want a wake word.

Note: Here you can also choose to customize the emojis you need by selecting “Emoji Collection”. Otherwise, there will be no emojis. You also need to select a preset font; otherwise, the dialogue text will not be fully displayed.

<img width="501" height="252" alt="image" src="https://github.com/user-attachments/assets/4b950a15-49c5-4829-947f-7274f5634abd" />

##### 25. After selecting, click “Next”.

<img width="554" height="276" alt="image" src="https://github.com/user-attachments/assets/b20c4d09-03a6-4a78-8d2c-3781dd3f94ba" />

##### 26. Click “Generate assets.bin”.


<img width="554" height="363" alt="image" src="https://github.com/user-attachments/assets/68ddd2e9-fa76-4c91-a8ff-0d265628be18" />

##### 27. Start Generate.

<img width="554" height="461" alt="image" src="https://github.com/user-attachments/assets/2f731c9f-fac8-447a-b42a-b6217ab94dd6" />


##### 28. Click “Flash to Device Online” to perform the remote update. After the update is completed, you can use the new wake word to wake up the device.

<img width="539" height="479" alt="image" src="https://github.com/user-attachments/assets/8b54c821-7d7e-4716-b583-a1ecf25fc2f7" />

##### 29. During the update process, please do not disconnect the device from power or the network.

<img width="542" height="346" alt="image" src="https://github.com/user-attachments/assets/e5f87c0c-ca5b-4a6b-83b6-8af65bb88b66" />



