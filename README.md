# Dissertation-Repo

https://os.mbed.com/users/liamcox94/code/sw_encrypt_loraWAN_implementation/

If you follow the link above then this will lead you to the repo for the software based encryption LoRaWAN implementation (Sorry but Github did not like the size of the files). This can be installed using the following method:
```
  $ source activate mbed-os2
  (mbed-os2)$ mbed import https://os.mbed.com/users/liamcox94/code/sw_encrypt_loraWAN_implementation/
  (mbed-os2)$ cd sw_encrypt_loraWAN_implementation
  (mbed-os2)$ mbed compile -t GCC_ARM -m K64F -f
 ```
 
To change to the hardware based encryption then download the file, LoRaMacCrypto.cpp and navigate into the mbed-os library in the implementation you just downloaded. The find the file with the same name, LoRaMacCrypto.cpp, and simple replace it. 
  

