/*---------------------------------------------------------------------------
 *      p2k-core.c
 * 
 *      V_1.5    2008/jan/25
 * 
 *      Copyright 2007 Sandor Otvos <s5vi.hu@gmail.com>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */
#include <usb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <termios.h>
#include <fcntl.h>
//---------------------------------------------------------------------------
//
// Globals vars
//
	int isVerbose, isSlave, isDevice, isModem, isLAN;
//---------------------------------------------------------------------------
//
// Usb globals vars
//
	int Vid, nC, nI, nA, nE, ATep_in, ATep_out, AT_ifnum, TCep_out, TCep_in, TC_ifnum, MEM_ifnum, MEMep_out, MEMep_in;
	struct usb_bus *bus;
	struct usb_device *dev;
//---------------------------------------------------------------------------
//
// P2k global variables
//
	uint16_t PacketID=0;
	char LastVol[256], VolumeList[34], PhoneModel[34], ACMdevice[256], LANcommand[64];
	uint8_t PacketCount[34], Recbuf[0x4000];
	int FilesNumber, FreeSpace, RecordSize, StayQuiet;
/*---------------------------------------------------------------------------
 * 
 * name: Power
 * @param base,int
 * @return int
 */
int power(int base, int n)
{
	int i, p;
	if (n == 0) return 1;
	p = 1;
	for (i = 1; i <= n; ++i) p = p * base;
	return p;
}
/*---------------------------------------------------------------------------
 * 
 * name: Hex string to int
 * @param pBuffer
 * @return int
 */
int htoi(char s[]) 
	{
	int len, value = 1, digit = 0,  total = 0;
	int c, x, y, i = 0;
	char hexchars[] = "abcdef"; /* Helper string to find hex digit values */
	len = strlen(s);
	for (x = i; x < len; x++) {
		c = tolower(s[x]);
		if (c >= '0' && c <= '9') {
			digit = c - '0';
		} else if (c >= 'a' && c <= 'f') {
			for (y = 0; hexchars[y] != '\0'; y++) {
				if (c == hexchars[y]) {
					digit = y + 10;
				}
			}
		} else { return 0; /* Return 0 if we get something unexpected */ }
			value = power(16, len-x-1);
			total += value * digit;
	}
	return total;
}
/*---------------------------------------------------------------------------
 * 
 * name: Print array in hex mode
 * @param headertext,pBuffer,size
 * if size==0 do nothing
 */
static void showhex(const char *header, const unsigned char *bytes, const int size)
{
	int i,j;
	char szBuf[17];
	for (j=0; j<17; j++) szBuf[j]=0;
	if (size==0) return;
	fprintf(stderr,"%s",header);
	if (header[1]=='S') {
		for (i=0; i<16; i++)
		{
			fprintf(stderr,"%3X",i);
		}
		fprintf(stderr," ASCII characters\n");
	}
	fprintf(stderr,"\n");
	for (i=0; i<size; i++)
	{
		if (i % 16 ==0)
		{
			if (i!=0) 	{ fprintf(stderr,"%s\n",szBuf);//fprintf(stderr,"\n");
				for (j=0; j<17; j++) szBuf[j]=0;
			}
			fprintf(stderr,"    %06x: ",i);
		}
		fprintf(stderr,"%02x ",bytes[i]);
		szBuf[i%16]=bytes[i];
		if (szBuf[i%16]<' ') szBuf[i%16]='.';
	}
	if (i%16!=0) for (j=0; j<(16-i%16); j++) fprintf(stderr,"   ");
	fprintf(stderr,"%s\n",szBuf);
}

static inline void showhex(const char* header, const char* bytes, const int size)
{
	showhex(header, (const unsigned char*)bytes, size);
}

/*---------------------------------------------------------------------------
 * 
 * name: set/get integer data into/from buffer
 * @param pBuffer,value
 * @return
 */
uint16_t getInt16( uint8_t *s)
{
	uint16_t res;
	res=s[0];
	res<<=8; res+=s[1];
	return (res);
}

uint32_t getInt32( uint8_t *s)
{
	uint32_t res;
	res=s[0];
	res<<=8; res+=s[1];
	res<<=8; res+=s[2];
	res<<=8; res+=s[3];
	return (res);
}

void setInt16(unsigned char *s, uint16_t value)
{
	s[1]=value%0x100; value/=0x100;
	s[0]=value%0x100;
}

void setInt32(unsigned char *s, uint32_t value)
{
	s[3]=value%0x100; value/=0x100;
	s[2]=value%0x100; value/=0x100;
	s[1]=value%0x100; value/=0x100;
	s[0]=value%0x100;
}
/////////////////////////////////////////////////////////////////////////////
//
// Libusb related low level commands
//
/////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------
 * 
 * name: open_dev if isDevice=1 search only vendor devices specified in Vid
 * @param interface name to search for 
 * @return device handle or 0 if not found 
 */
usb_dev_handle *open_dev(char *tmp)
{
	struct usb_dev_handle *curr_dev;
	int i;
	char string[256];

	for(bus = usb_get_busses(); bus; bus = bus->next) {
		for(dev = bus->devices; dev; dev = dev->next) {
			// check if Vendor specified?
			//if (isDevice==1) if (dev->descriptor.idVendor!=Vid) continue;
		if (Vid!=0) { if (dev->descriptor.idVendor!=Vid) continue; }
			// get i/f details
			curr_dev=usb_open(dev);
			if (curr_dev>0) {
#ifndef WIN32
				if(!dev->config) i=usb_fetch_and_parse_descriptors(curr_dev); else i = 1;
#endif				  
				//i=usb_fetch_and_parse_descriptors(curr_dev);
				if (i>0) {
					if (isVerbose==1) {
						i = usb_get_string_simple(curr_dev, dev->descriptor.iProduct,string, sizeof(string));
						if (i>0) fprintf(stderr,"_Found: %s\n",string);
					}
					for (nC=0;nC<dev->descriptor.bNumConfigurations;nC++) {
						for (nI=0;nI<dev->config[nC].bNumInterfaces;nI++) {
							for (nA=0;nA<dev->config[nC].interface[nI].num_altsetting;nA++) {
								i = usb_get_string_simple(curr_dev, dev->config[nC].interface[nI].altsetting[nA].iInterface, string, sizeof(string));
								if (i > 0) {
									//fprintf(stderr,"_Found: %s\n",string);
//									if (strcmp (tmp,string)==0) {
									if (strncmp (tmp,string,strlen(tmp))==0) {
									//if (strstr (string,tmp)!=0) {
										fprintf(stderr," Found Interface: %s\n",string);
										return curr_dev;
									} 
								}
							}
						}
					}
				}
				usb_close(curr_dev);
			}
		}
	}
return NULL;
}
/*---------------------------------------------------------------------------
 * 
 * name: Find_TC_device
 * @param
 * @return device handle or 0 if not found 
 */
usb_dev_handle *Find_TC_device ()
{
usb_dev_handle *dev=NULL;  /* the device handle */
	if(!(dev = open_dev("Motorola Test Command"))) {
		return 0;
	}
	return dev;
}
/*---------------------------------------------------------------------------
 * 
 * name: Find_USBLAN_device
 * @param
 * @return device handle or 0 if not found 
 */
usb_dev_handle *Find_USBLAN_device ()
{
usb_dev_handle *dev=NULL;  /* the device handle */
	if(!(dev = open_dev("Belcarra USBLAN - MDLM/BLAN"))) {
		return 0;
	}
	return dev;
}
/*---------------------------------------------------------------------------
 * 
 * name: Find_MEM_device
 * @param
 * @return device handle or 0 if not found 
 */
usb_dev_handle *Find_MEM_device ()
{
usb_dev_handle *dev=NULL;  /* the device handle */
	if(!(dev = open_dev("Motorola Mass Storage Interface"))) {
		return 0;
	}
#ifdef LINUX
//
// This is need only linux
// Detach kernel driver
//
	char oldDriver[1024];
	int ret,i;
	//Just hack.. will fix this later.
	for (i=0; i<20; i++) {
		ret=usb_get_driver_np(dev,i,oldDriver,sizeof(oldDriver)-1);
		if (ret>=0) {
			//Interface is claimed;
			fprintf(stderr," Interface %02x is claimed by kernel driver: %s\n",i,oldDriver);
			ret=usb_detach_kernel_driver_np(dev, i);
			if (ret>=0) {
				fprintf(stderr," Interface %02x is detached from kernel driver: %s\n",i,oldDriver);
			}
			else {
				fprintf(stderr,"!Unable to detach kernel driver: %s\n",oldDriver);
				usb_close(dev);
				return 0;
			}
		}
	}
#endif
#ifdef DARWIN
	fprintf(stderr,"On OSX USBSTORAGE kernel driver must be unload,\n please enter admin password for sudo.\n");
	system("sudo kextunload /System/Library/Extensions/IOUSBFamily.kext/Contents/PlugIns/AppleUSBCDC*.kext");
#endif
	return dev;
}
/*---------------------------------------------------------------------------
 * 
 * name: Find_AT_device
 * @param
 * @return device handle or 0 if not found 
 */
usb_dev_handle *Find_AT_device ()
{
usb_dev_handle *dev=NULL;  /* the device handle */
	if(!(dev = open_dev("Motorola Data Interface"))) {
		if(!(dev = open_dev("Motorola Communication Interface"))) {
		//fprintf(stderr,"!error: not found!\n");
		return 0;
		}
	}
//---------------------------------------------------------------------------
//
// This is not need for linux/darwin
//
//if(usb_set_configuration(dev, 1) < 0) {
//	fprintf(stderr,"error: setting config 1 failed\n");
//	usb_close(dev);
//	return 0;
//}

#ifdef LINUX
//
// This is need only linux
// Detach kernel driver
//
	char oldDriver[1024];
	int ret,i;
	//Just hack.. will fix this later.
	for (i=0; i<20; i++) {
		ret=usb_get_driver_np(dev,i,oldDriver,sizeof(oldDriver)-1);
		if (ret>=0) {
			//Interface is claimed;
			fprintf(stderr," Interface %02x is claimed by kernel driver: %s\n",i,oldDriver);
			ret=usb_detach_kernel_driver_np(dev, i);
			if (ret>=0) {
				fprintf(stderr," Interface %02x is detached from kernel driver: %s\n",i,oldDriver);
			}
			else {
				fprintf(stderr,"!Unable to detach kernel driver: %s\n",oldDriver);
				usb_close(dev);
				return 0;
			}
		}
	}
#endif
#ifdef DARWIN
	fprintf(stderr,"On OSX CDCACM kernel driver must be unload,\n please enter admin password for sudo.\n");
	system("sudo kextunload /System/Library/Extensions/IOUSBFamily.kext/Contents/PlugIns/AppleUSBCDC*.kext");
#endif
	return dev;
}
/*---------------------------------------------------------------------------
 * 
 * name: Switch_ATtoP2kmode
 * @param
 * @return 1:ok, 0:error
 */
int Switch_ATtoP2kmode (usb_dev_handle *hdev)
{
	int i;
	
	fprintf (stderr," Switching to P2kmode (cca. 2-3 sec) \n");
	if (isVerbose==1) showhex("_Sent  Data","AT+MODE=8\r\n", 11);
	i=usb_bulk_write(hdev, ATep_out,"AT+MODE=8\r\n", 11, 5000);
	if(i<1) {
		fprintf(stderr,"!error: bulk write failed\n");
		usb_close(hdev);
		return 0;
	}

	//i=usb_bulk_read(hdev, ATep_in, tmp,sizeof(tmp), 5000);
	//if(i<0) {
		//fprintf(stderr,"!error: bulk read failed\n");
		//usb_close(hdev);
		//return 0;
	//} else { tmp[i-1]=0; fprintf (stderr," Received echo: %s\n",i); }
	//if (isVerbose==1) showhex("Recvd Data",tmp,i);
	return 1;
}
/*---------------------------------------------------------------------------
 * 
 * name: Switch_ModemtoP2kmode
 * @param
 * @return 1:ok, 0:error
 */
int Switch_ModemtoP2kmode ()
{
	int t;
	char ACMcmd[50];

	fprintf (stderr," Switching to P2kmode (cca. 2-3 sec) \n");
	if (isVerbose==1) showhex("_Sent  Data","AT+MODE=8\r\n", 11);
#ifndef WIN32
	int acm;
	acm=open(ACMdevice, O_RDWR | O_NOCTTY | O_NONBLOCK );
	if (acm<0)	{ fprintf(stderr,"!error: Modem device not found.\n"); return 0;}
	
	struct termios tio;
	bzero(&tio, sizeof(tio));
	tio.c_cflag= CRTSCTS | CS8 | CLOCAL | CREAD | O_NDELAY;
	cfsetispeed(&tio, B115200);
	tio.c_cflag &= ~CRTSCTS;
	tio.c_iflag = IGNPAR;
	tio.c_oflag = 0;
	tio.c_lflag = 0;
	tio.c_cc[VTIME] = 1;
	tio.c_cc[VMIN] = 0;
	tcflush(acm, TCIOFLUSH);
	tcsetattr(acm, TCSANOW, &tio);
	char readBuf[1024];
	int nread;
	
	strcpy(ACMcmd,"AT E0\r\n");
	write(acm, ACMcmd, strlen(ACMcmd));
	tcdrain(acm);
	
	nread=0; t=time(NULL);
	while (time(NULL)-t<5) {
	    nread=read(acm,readBuf,sizeof(readBuf)-1);
	    if (nread>0) break;
		usleep(1000);
	}
	if (nread>0)
	{
	    readBuf[nread]=0;
//	    Message("%1 answer: %2").arg(ACMcmd).arg(readBuf));
	}
	

	strcpy(ACMcmd,"AT+MODE=8\r\n");
	write(acm, ACMcmd, strlen(ACMcmd));
	tcdrain(acm);
	
	nread=0; t=time(NULL);
	while (time(NULL)-t<5) {
	    nread=read(acm,readBuf,sizeof(readBuf)-1);
	    if (nread>0) break;
		usleep(1000);
	}

	if (nread>0)
	{
	    readBuf[nread]=0;
	//  Message("Phone answer: %1").arg(readBuf));
	}
	else
	{
	//    error("Unable to connect");
	//    Message("nread=%1, errno=%2").arg(nread).arg(errno));
	}
	close(acm);
	sleep(1);
#else
  FILE* acm;
  DCB dcb;
  acm = CreateFileA ( ACMdevice,
                    GENERIC_READ | GENERIC_WRITE,
                    0,    // must be opened with exclusive-access
                    NULL, // no security attributes
                    OPEN_EXISTING, // must use OPEN_EXISTING
                    0,    // not overlapped I/O
                    NULL  // hTemplate must be NULL for comm devices
                    );
   // Fill in DCB: 57,600 bps, 8 data bits, no parity, and 1 stop bit.
   dcb.BaudRate = CBR_57600;     // set the baud rate
   dcb.ByteSize = 8;             // data size, xmit, and rcv
   dcb.Parity = NOPARITY;        // no parity bit
   dcb.StopBits = ONESTOPBIT;    // one stop bit
   SetCommState(acm, &dcb);
   char s[100]="AT+MODE=8\r\n";
   unsigned long l1=strlen(s);
   unsigned long l2;
   WriteFile(acm, s, l1, &l2, 0);
#endif
	return 1;
}
/*---------------------------------------------------------------------------
 * 
 * name: Switch_ModemtoLANmode
 * @param
 * @return 1:ok, 0:error
 */
int Switch_ModemtoLANmode ()
{
	int t;
	char ACMcmd[50];

	if (strlen(LANcommand)==0) strcpy(LANcommand,"AT+MODE=8\r\n");
	else strcat(LANcommand,"\r\n");
	fprintf (stderr," Switching to USBLAN mode (cca. 2-3 sec) \n");
	if (isVerbose==1) showhex("_Sent  Data",LANcommand,strlen(LANcommand));
#ifndef WIN32
	int acm;
	acm=open(ACMdevice, O_RDWR | O_NOCTTY | O_NONBLOCK );
	if (acm<0)	{ fprintf(stderr,"!error: Modem device not found.\n"); return 0;}
	
	struct termios tio;
	bzero(&tio, sizeof(tio));
	tio.c_cflag= CRTSCTS | CS8 | CLOCAL | CREAD | O_NDELAY;
	cfsetispeed(&tio, B115200);
	tio.c_cflag &= ~CRTSCTS;
	tio.c_iflag = IGNPAR;
	tio.c_oflag = 0;
	tio.c_lflag = 0;
	tio.c_cc[VTIME] = 1;
	tio.c_cc[VMIN] = 0;
	tcflush(acm, TCIOFLUSH);
	tcsetattr(acm, TCSANOW, &tio);
	char readBuf[1024];
	int nread;
	
	strcpy(ACMcmd,"AT E0\r\n");
	write(acm, ACMcmd, strlen(ACMcmd));
	tcdrain(acm);
	
	nread=0; t=time(NULL);
	while (time(NULL)-t<5) {
	    nread=read(acm,readBuf,sizeof(readBuf)-1);
	    if (nread>0) break;
		usleep(1000);
	}
	if (nread>0)
	{
	    readBuf[nread]=0;
//	    Message("%1 answer: %2").arg(ACMcmd).arg(readBuf));
	}
	

	strcpy(ACMcmd,LANcommand);
	write(acm, ACMcmd, strlen(ACMcmd));
	tcdrain(acm);
	
	nread=0; t=time(NULL);
	while (time(NULL)-t<5) {
	    nread=read(acm,readBuf,sizeof(readBuf)-1);
	    if (nread>0) break;
		usleep(1000);
	}

	if (nread>0)
	{
	    readBuf[nread]=0;
	//  Message("Phone answer: %1").arg(readBuf));
	}
	else
	{
	//    error("Unable to connect");
	//    Message("nread=%1, errno=%2").arg(nread).arg(errno));
	}
	close(acm);
	sleep(1);
#else
  FILE *acm;
  DCB dcb;
  acm = CreateFileA ( ACMdevice,
                    GENERIC_READ | GENERIC_WRITE,
                    0,    // must be opened with exclusive-access
                    NULL, // no security attributes
                    OPEN_EXISTING, // must use OPEN_EXISTING
                    0,    // not overlapped I/O
                    NULL  // hTemplate must be NULL for comm devices
                    );
   // Fill in DCB: 57,600 bps, 8 data bits, no parity, and 1 stop bit.
   dcb.BaudRate = CBR_57600;     // set the baud rate
   dcb.ByteSize = 8;             // data size, xmit, and rcv
   dcb.Parity = NOPARITY;        // no parity bit
   dcb.StopBits = ONESTOPBIT;    // one stop bit
   SetCommState(acm, &dcb);
   //char s[100]="AT+MODE=13\r\n";
   //unsigned long l1=strlen(s);
   unsigned long l2;
   WriteFile(acm, LANcommand,strlen(LANcommand), &l2, 0);
#endif
	return 1;
}
/*---------------------------------------------------------------------------
 * 
 * name: Switch_MemtoP2kmode
 * @param
 * @return 1:ok, 0:error
 */
int Switch_MEMtoP2kmode (usb_dev_handle *hdev)
{
	char cmd[]={
	0x55, 0x53, 0x42, 0x43, 0x08, 0xC0, 0xD9, 0x85,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0xD6,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00 };
	int ret;
	fprintf (stderr," Switching to P2kmode (cca. 2-3 sec) \n");
	ret=usb_bulk_write(hdev, MEMep_out,cmd,sizeof(cmd), 1000);
	if (ret<1) { fprintf(stderr,"!error: bulk write failed\n"); return 0; }
	if (isVerbose==1) showhex("_Sent  Data",cmd,ret);
	//i=usb_bulk_read(hdev, MEMep_in, tmp,sizeof(tmp), 1000);
	//if(i<1) {
		//fprintf(stderr,"!error: bulk read failed\n");
		//usb_close(hdev);
		//return 0;
	//} else { tmp[i+1]=0; fprintf (stderr," Received echo: %s\n",i); }
	//if (isVerbose==1) showhex("Recvd Data",tmp,i);
	return 1;
}
/*---------------------------------------------------------------------------
 * 
 * name: Switch_ATtoMemmode
 * @param
 * @return 1:ok, 0:error
 */
int Switch_ATtoMemmode (usb_dev_handle *hdev)
{
	printf (" Switching to Memcard mode (cca. 2-3 sec) \n");
	if (isVerbose==1) showhex("_Sent  Data","AT+MODE=24\r\n", 12);
	if(usb_bulk_write(hdev, ATep_out,"AT+MODE=24\r\n", 12, 5000) != 12) {
		fprintf(stderr,"!error: bulk write failed\n");
		usb_close(hdev);
		return 0;
	}

	//i=usb_bulk_read(hdev, ATep_in, tmp,sizeof(tmp), 5000);
	//if(i<0) {
		//fprintf(stderr,"!error: bulk read failed\n");
		//usb_close(hdev);
		//return 0;
	//} else { tmp[i+1]=0; printf (" Received echo: %s\n",i); }
	//if (isVerbose==1) showhex("Recvd Data",tmp,i);
	return 1;
}
/*---------------------------------------------------------------------------
 * 
 * name: Switch_ATtoLANmode
 * @param
 * @return 1:ok, 0:error
 */
int Switch_ATtoLANmode (usb_dev_handle *hdev)
{
	if (strlen(LANcommand)==0) strcpy(LANcommand,"AT+MODE=8\r\n");
	else strcat(LANcommand,"\r\n");
	printf (" Switching to USBLAN mode (cca. 2-3 sec) \n");
	if (isVerbose==1) showhex("_Sent  Data",LANcommand,strlen(LANcommand));
	if(usb_bulk_write(hdev, ATep_out,LANcommand,strlen(LANcommand), 5000) != 12) {
		fprintf(stderr,"!error: bulk write failed\n");
		usb_close(hdev);
		return 0;
	}
	return 1;
}
/*---------------------------------------------------------------------------
 * 
 * name: Switch_P2ktoATmode
 * @param
 * @return 1:ok, 0:error
 */
int Switch_P2ktoATmode (usb_dev_handle *hdev)
{
	//unsigned char tmp[]={
	//0x00, 0x00, 0x00 };
	int ret;
	printf (" Switching to AT mode (cca. 2-3 sec) \n");
	//ret=usb_control_msg(hdev, 0x40, 0x01, 0x00, 0x01, tmp, sizeof(tmp), 1000);
	ret=usb_control_msg(hdev, 0x40, 0x01, 0x00, 0x01, 0, 0, 1000);
	if (ret<0) { printf ("!error sending ctrl data.\n"); return 0; }
	usb_release_interface(hdev, TC_ifnum);
	//if (isVerbose==1) showhex("_Sent  Data",tmp,ret);
	//t=time(NULL);
	//// Read status
	//while (time(NULL)-t<5)
	//{
		//ret=usb_control_msg(hdev,0xc1, 0x00, 0x00,TC_ifnum,PacketCount,sizeof(PacketCount),1000);
		//if (isVerbose==1) showhex("_Recvd Stat",PacketCount,ret);
		//if (ret>2) break;
		//usleep(100);
	//}
	//if (ret<=2) { printf ("!timeout receiving status.\n"); return 0; }// Timeout
	//// Calculate answer size
	//recsize=get_cmd_size(PacketCount);
	//// Read answer
	//ret=usb_control_msg(hdev,0xc1, 0x01, 0x01,TC_ifnum,Recbuf,recsize,1000);
	//if (ret<=0) { printf ("!error: receiving ctrl data.\n"); return 0; }
	//if (isVerbose==1) showhex("_Recvd Data",Recbuf,ret);	return 1;
	return 1;
}
/*---------------------------------------------------------------------------
 * 
 * name: P2k_Connect
 * @param 
 * @return  device handle  0:not found
 */
usb_dev_handle *P2k_Connect (usb_dev_handle *hdev)
{
	//usb_dev_handle *hdev = NULL; /* the device handle */
	int i,t;
	char s[256];
	FILE *pFile;
	
	//if (hdev!=0) return hdev; // already connected
	if (hdev>0) usb_close(hdev);
	hdev=NULL;
	// check if already in USBLAN mode
	fprintf(stderr," Search for Motorola USBLAN Interface.\n");
	hdev=Find_USBLAN_device();
	if (hdev!=0) {
		system("ipconfig /all");
		return hdev;
	}
	// check if already in P2k mode
	fprintf(stderr," Search for Motorola Test Command.\n");
	hdev=Find_TC_device();
	if (hdev==0) {
		if (isModem==1) { fprintf(stderr," Modem mode, device: %s\n",ACMdevice);
			if (!strcmp (ACMdevice,"auto")) {
				fprintf (stderr," Auto-detecting modem device: ");
				strcpy(ACMdevice,"/dev/");
#ifdef DARWIN
				system ("ls /dev | grep usbmodem >MODEM");
#else
				system ("ls /dev | grep ACM >MODEM");
#endif
				pFile = fopen ("MODEM" , "r");fgets (s , 100 , pFile);fclose (pFile);
				strcat(ACMdevice,s);
				fprintf (stderr,"%s",ACMdevice);ACMdevice[strlen(ACMdevice)-1]=0;
			}
			//showhex("---",ACMdevice,32);
			i=Switch_ModemtoP2kmode();
			if (i==0) { fprintf(stderr,"!error: device not found.\n"); return 0;}
				sleep(1);
				t=time(NULL);
				//////if (isVerbose==1) isVerbose=2;
				// loop, waiting to TC device appear
				fprintf(stderr," Search for Motorola Test Command.\n");
				while (time(NULL)-t<5) {
					usb_find_devices(); /* find all connected devices */
					hdev=Find_TC_device();
					if (hdev!=0) break;
					usleep(1000);
				}
				if (isVerbose==2) isVerbose=1;
				if (hdev==0) return 0; // TC not appeared
			}
		if (isModem==0) {
			fprintf(stderr," Search for Motorola Data Interface.\n");
			hdev=Find_AT_device();
			if (hdev!=0) {
				// get AT device info and switch
				ATep_out=dev->config[nC].interface[nI].altsetting[nA].endpoint[0].bEndpointAddress;
				// check endpoint order
				if (ATep_out<0x80) ATep_in=dev->config[nC].interface[nI].altsetting[nA].endpoint[1].bEndpointAddress;
				else {
					ATep_out=dev->config[nC].interface[nI].altsetting[nA].endpoint[1].bEndpointAddress;
					ATep_in=dev->config[nC].interface[nI].altsetting[nA].endpoint[0].bEndpointAddress;
				}
				AT_ifnum=dev->config[nC].interface[nI].altsetting[nA].bInterfaceNumber;
				fprintf (stderr," Inteface number: %02x, Endpoints: %02x %02x\n",AT_ifnum,ATep_in,ATep_out);
				if(usb_claim_interface(hdev, AT_ifnum) < 0) {
					fprintf(stderr,"!error: claiming interface 1 failed\n");
					usb_close(hdev);
					return 0;
				}
				i=Switch_ATtoP2kmode(hdev);
				if (i==0) return 0;
				usb_release_interface(hdev, AT_ifnum);
				usb_close(hdev);
				sleep(1);
				t=time(NULL);
				if (isVerbose==1) isVerbose=2;
				// loop, waiting to TC device appear
				fprintf(stderr," Search for Motorola Test Command.\n");
				while (time(NULL)-t<5) {
					usb_find_devices(); /* find all connected devices */
					hdev=Find_TC_device();
					if (hdev!=0) break;
					usleep(1000);
				}
				if (isVerbose==2) isVerbose=1;
				if (hdev==0) return 0; // TC not appeared
			} else { fprintf(stderr," Search for Motorola Mass Storage Interface.\n");
				hdev=Find_MEM_device();
				if (hdev==0) { fprintf(stderr,"!error: device not found.\n"); return 0; } // nor TC,AT,MEM devices found 
				// get MEM device info and switch
				// Now only warning msg.
				//fprintf(stderr,"Please set your phone Data/Fax mode.\n");
				//usb_close(hdev);
				//sleep(1);
				//hdev=Find_TC_device();
				//if (hdev==0) return 0; // TC not appeared
				// Later try to switch
				MEMep_out=dev->config[nC].interface[nI].altsetting[nA].endpoint[0].bEndpointAddress;
				// check endpoint order
				if (MEMep_out<0x80) MEMep_in=dev->config[nC].interface[nI].altsetting[nA].endpoint[1].bEndpointAddress;
				else {
					MEMep_out=dev->config[nC].interface[nI].altsetting[nA].endpoint[1].bEndpointAddress;
					MEMep_in=dev->config[nC].interface[nI].altsetting[nA].endpoint[0].bEndpointAddress;
				}
				MEM_ifnum=dev->config[nC].interface[nI].altsetting[nA].bInterfaceNumber;
				fprintf (stderr," Inteface number: %02x, Endpoints: %02x %02x\n",MEM_ifnum,MEMep_in,MEMep_out);
				if(usb_claim_interface(hdev, MEM_ifnum) < 0) {
					fprintf(stderr,"!error: claiming interface 1 failed\n");
					usb_close(hdev);
					return 0;
				}
				Switch_MEMtoP2kmode(hdev);
				//usb_release_interface(hdev, MEM_ifnum);
				//usb_close(hdev);
				sleep(1);
				t=time(NULL);
				if (isVerbose==1) isVerbose=2;
				// loop, waiting to TC device appear
				fprintf(stderr," Search for Motorola Test Command.\n");
				while (time(NULL)-t<5) {
					usb_find_devices(); /* find all connected devices */
					hdev=Find_TC_device();
					if (hdev!=0) break;
					usleep(1000);
				}
				if (isVerbose==2) isVerbose=1;
				if (hdev==0) return 0; // TC not appeared
			}
		}
	}
	// get TC device info 
	if (dev->config[nC].interface[nI].altsetting[nA].bNumEndpoints!=0) {
		TCep_out=dev->config[nC].interface[nI].altsetting[nA].endpoint[0].bEndpointAddress;
		// check endpoint order
		if (TCep_out<0x80) TCep_in=dev->config[nC].interface[nI].altsetting[nA].endpoint[1].bEndpointAddress;
		else {
			TCep_out=dev->config[nC].interface[nI].altsetting[nA].endpoint[1].bEndpointAddress;
			TCep_in=dev->config[nC].interface[nI].altsetting[nA].endpoint[0].bEndpointAddress;
		}
	}
	TC_ifnum=dev->config[nC].interface[nI].altsetting[nA].bInterfaceNumber;
	fprintf (stderr," Inteface number: %02x, Endpoints: %02x %02x\n",TC_ifnum,TCep_in,TCep_out);
	if (TCep_in==0) fprintf(stderr," This is P2k phone.\n");
	else fprintf(stderr," This is P2k05 phone.\n");
#ifndef WIN32	  
	if(usb_claim_interface(hdev, TC_ifnum) < 0) {
		fprintf(stderr,"!error: claiming interface failed\n");
		usb_close(hdev);
		return 0;
	}
#endif	
	return hdev;
}
/*---------------------------------------------------------------------------
 * 
 * name: P2k_ConnectLAN
 * @param 
 * @return  device handle  0:not found
 */
usb_dev_handle *P2k_ConnectLAN (usb_dev_handle *hdev)
{
	//usb_dev_handle *hdev = NULL; /* the device handle */
	int i,t;
	char s[256];
	FILE *pFile;
	//if (hdev!=0) return hdev; // already connected
	if (hdev>0) usb_close(hdev);
	hdev=NULL;
	fprintf(stderr," Search for Motorola USBLAN device.\n");
	hdev=Find_USBLAN_device();
	if (hdev==0) {
		if (isModem==1) { fprintf(stderr," Modem mode, device: %s\n",ACMdevice);
			if (!strcmp (ACMdevice,"auto")) {
				fprintf (stderr," Auto-detecting modem device: ");
				strcpy(ACMdevice,"/dev/");
#ifdef DARWIN
				system ("ls /dev | grep usbmodem >MODEM");
#else
				system ("ls /dev | grep ACM >MODEM");
#endif
				pFile = fopen ("MODEM" , "r");fgets (s , 100 , pFile);fclose (pFile);
				strcat(ACMdevice,s);
				fprintf (stderr,"%s",ACMdevice);ACMdevice[strlen(ACMdevice)-1]=0;
			}
			//showhex("---",ACMdevice,32);
			i=Switch_ModemtoLANmode();
			if (i==0) { fprintf(stderr,"!error: device not found.\n"); return 0;}
				sleep(1);
				t=time(NULL);
				/////if (isVerbose==1) isVerbose=2;
				// loop, waiting to TC device appear
				fprintf(stderr," Search for Motorola USBLAN device.\n");
				while (time(NULL)-t<5) {
					usb_find_devices(); /* find all connected devices */
					hdev=Find_USBLAN_device();
					if (hdev!=0) break;
					usleep(1000);
				}
				if (isVerbose==2) isVerbose=1;
				if (hdev==0) return 0; // USBLAN not appeared
				system ("ipconfig /all");
			}
		if (isModem==0) {
			fprintf(stderr," Search for Motorola USBLAN device.\n");
			hdev=Find_AT_device();
			if (hdev!=0) {
				// get AT device info and switch
				ATep_out=dev->config[nC].interface[nI].altsetting[nA].endpoint[0].bEndpointAddress;
				// check endpoint order
				if (ATep_out<0x80) ATep_in=dev->config[nC].interface[nI].altsetting[nA].endpoint[1].bEndpointAddress;
				else {
					ATep_out=dev->config[nC].interface[nI].altsetting[nA].endpoint[1].bEndpointAddress;
					ATep_in=dev->config[nC].interface[nI].altsetting[nA].endpoint[0].bEndpointAddress;
				}
				AT_ifnum=dev->config[nC].interface[nI].altsetting[nA].bInterfaceNumber;
				fprintf (stderr," Inteface number: %02x, Endpoints: %02x %02x\n",AT_ifnum,ATep_in,ATep_out);
				if(usb_claim_interface(hdev, AT_ifnum) < 0) {
					fprintf(stderr,"!error: claiming interface 1 failed\n");
					usb_close(hdev);
					return 0;
				}
				i=Switch_ATtoLANmode(hdev);
				if (i==0) return 0;
				usb_release_interface(hdev, AT_ifnum);
				usb_close(hdev);
				sleep(1);
				t=time(NULL);
				if (isVerbose==1) isVerbose=2;
				// loop, waiting to USBLAN device appear
				fprintf(stderr," Search for Motorola USBLAN device.\n");
				while (time(NULL)-t<5) {
					usb_find_devices(); /* find all connected devices */
					hdev=Find_USBLAN_device();
					if (hdev!=0) break;
					usleep(1000);
				}
				if (isVerbose==2) isVerbose=1;
				if (hdev==0) return 0; // USBLAN not appeared
				system ("ipconfig /all");
			} 
		}
	}
	// get USBLAN device info 
	return hdev;
}
/*---------------------------------------------------------------------------
 * 
 * name: get_cmd_size
 * @param received PacketCount 
 * @return  Number of received bytes
 */
int get_cmd_size(unsigned char *packetCount)
{
	uint16_t cnt;
	uint16_t size;
	int bufsize;
	cnt=getInt16(packetCount);
	size=getInt16(packetCount+2);
	bufsize=2*cnt+size+4;
	return(bufsize);
}
/*---------------------------------------------------------------------------
 * 
 * name: P2k_SendCommand
 * @param hdev,pSendbuf,Sendsize,pRecbuf,Recsize
 * @return  Number of received bytes, -1:error
 */
int P2k_SendCommand (usb_dev_handle *hdev,unsigned char *Sendbuf,unsigned int Sendsize,unsigned char *Recbuf) 
{
	int ret,t,recsize;
	unsigned char P2k05buf [0x1005];
	uint16_t recPID;
	// prepare Sendbuf content
	// increment PacketID
	PacketID++;
	PacketID&=0xFFF;
	// insert PacketID into the beginning of Sendbuf
	setInt16(Sendbuf,PacketID);
	// Decide P2k or P2k05
	if (TCep_in==0) { // P2k (old)
		// SendSendbuf
		///////////////////////// req,rqtype,value,index,packet
		if (isVerbose==1) showhex("_Sent  Data",Sendbuf,Sendsize);
		ret=usb_control_msg(hdev, 0x41, 0x02, 0x00, TC_ifnum, (char*)Sendbuf, Sendsize, 1000);
		if (ret<0) { printf ("!error sending ctrl data.\n"); return -1; }
		t=time(NULL);
		// Read status
		while (time(NULL)-t<5)
		{
			ret=usb_control_msg(hdev,0xc1, 0x00, 0x00,TC_ifnum,(char*)PacketCount,sizeof(PacketCount),1000);
			if (isVerbose==1) showhex("_Recvd Stat",PacketCount,ret);
			if (ret>2) break;
			usleep(100);
		}
		if (ret<=2) { printf ("!timeout receiving status.\n"); return -1; }// Timeout
		// Calculate answer size
		recsize=get_cmd_size(PacketCount);
		// Read answer
		ret=usb_control_msg(hdev,0xc1, 0x01, 0x01,TC_ifnum,(char*)Recbuf,recsize,1000);
		if (ret<=0) { printf ("!error: receiving ctrl data.\n"); return -1; }
		if (isVerbose==1) showhex("_Recvd Data",Recbuf,ret);
		// Check answer
		if (Recbuf[0]!=1) { printf ("!error: received data corrupted.\n"); return -1; }
		if ((Recbuf[6]&0x80)!=0x80) { printf ("!error: received data corrupted.\n"); return -1; }
		t=Recbuf[8]&0xf0;
		if (t!=0x80 && t!=0x60 && StayQuiet==0) { printf ("!error: received data corrupted:%04x\n",t); return -1; }
		recPID=getInt16(Recbuf+6)&0x0FFF;
		if (recPID!=PacketID) { printf ("!error: packetID mismatch:%04x %04x\n",PacketID,recPID); return -1; }
		//if (!(getInt16(Recbuf+6)&0x0FFF!=PacketID)) { printf ("error: packetID mismatch.\n"); return 0; }
		return ret-14; //recsize;
	} else { // P2k05 (new)
		// Format Sendbuf to fit p2k05 cmd structure
		// dw:PacketID, dw:cmd, dw:0, dw:0, dw:0, dw:argsize, then content
		memcpy (P2k05buf,Sendbuf,4); // pid and cmd
		setInt16(P2k05buf+4,0); // zero separators
		setInt16(P2k05buf+6,0);
		setInt16(P2k05buf+8,0);
		memcpy (P2k05buf+10,Sendbuf+4,2); // argsize
		memcpy (P2k05buf+12,Sendbuf+8,Sendsize-8); // data payload
		// Send Sendbuf
		ret=usb_bulk_write(hdev, TCep_out,(char*)P2k05buf,Sendsize+4, 1000);
		if (ret<1) { printf ("!error sending bulk data.\n"); return -1; }
		if (isVerbose==1) showhex("_Sent  Data",P2k05buf,ret);
		
		ret=usb_bulk_read(hdev, TCep_in, (char*)P2k05buf,0x1000, 1000);
		if (ret<1) { printf ("!error reading P2k05 answer.\n"); return -1; }
		if (isVerbose==1) showhex("_Recvd Data",P2k05buf,ret);
		
		
		//// Waitfor and Read answer
		//t=time(NULL);
		//while (time(NULL)-t<5)
		//{
			//P2k05buf[0]=2;
			//P2k05buf[1]=0;
			//P2k05buf[2]=0x10;
			//P2k05buf[3]=0;
			//P2k05buf[4]=0;
			//// read status
			//ret=usb_bulk_write(hdev, TCep_out,P2k05buf,5, 1000);
			//if (ret<1) { printf ("!error sending query P2k05 status cmd.\n"); return 0; }
			//if (isVerbose==1) showhex("_Sent Query",P2k05buf,ret);
			//ret=usb_bulk_read(hdev, TCep_in, P2k05buf,0x1005, 1000);
			//if (ret<1) { printf ("!error reading P2k05 status.\n"); return 0; }
			//if (isVerbose==1) showhex("_Recvd Stat/Data",P2k05buf,ret);
			//if (ret>2) break;
			//usleep(100);
		//}
		//if (ret<=2) { printf ("!timeout querying P2k05 status.\n"); return 0; }// Timeout
		// Format Recbuf to fit p2k structure
		memcpy (Recbuf+2,P2k05buf,ret);
		return ret-12;
	}
}
/////////////////////////////////////////////////////////////////////////////
//
// P2k low level commands
//
/////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------
 * 
 * name: P2k_GetVolnames
 * @param hdev,target string 
 * @return  lenght
 */
int P2k_GetVolnames (usb_dev_handle *hdev)
{
	int ps,buf_ps;
	unsigned char Sendbuf[]={
	0xFF, 0xFF, // 2 byte packetID
	0x00, 0x4A, // P2k cmd (0x4A:FSAC)
	0x00, 0x04, // argsize (extra bytes)
	0x00, 0x00, // zero separator
	0x00, 0x00, 0x00, 0x0A}; // FSAC command 
							 //extra bytes (cmd args)
	if (P2k_SendCommand (hdev,Sendbuf,sizeof(Sendbuf),Recbuf)==-1) return 0;
	buf_ps=0;
	ps=0x0F;
	while (Recbuf[ps]!=0)
	{
		if (Recbuf[ps]==0xfe) Recbuf[ps]=32;
		VolumeList[buf_ps++]=Recbuf[ps];
		ps+=2;
	}
	VolumeList[buf_ps++]=0;
	return 1;
}
/*---------------------------------------------------------------------------
 * 
 * name: P2k_GetFreespace
 * @param hdev, volume string (eg: 'a' or 'b')
 * @return free space
 */
int P2k_GetFreespace(usb_dev_handle *hdev,unsigned char *Vol)
{
	unsigned char Sendbuf[0x108]={
	0xFF, 0xFF, // 2 byte packetID
	0x00, 0x4A, // P2k cmd (0x4A:FSAC)
	0x01, 0x00, // argsize (extra bytes)
	0x00, 0x00, // zero separator
	0x00, 0x00, 0x00, 0x0B,	// FSAC command
	0x00, 0x00, 
	0x00, '/' , // extra bytes (cmd args)
	0x00, 'a' ,
	0x00, 0x00};
	Sendbuf[17]=Vol[0];
	if (P2k_SendCommand (hdev,Sendbuf,sizeof(Sendbuf),Recbuf)==-1) return 0;
	return(getInt32(Recbuf+14));
}
/*---------------------------------------------------------------------------
 * 
 * name: P2k_GetFilesNumber
 * @param hdev,Volume,Filter string
 * @return number
 */
int P2k_GetFilesNumber(usb_dev_handle *hdev,unsigned char *Vol)
{
	unsigned int i,j;
	//unsigned char Sendbuf[strlen(Vol)+18];
	unsigned char Sendbuf[0x108]={
	0xFF, 0xFF, // 2 byte packetID
	0x00, 0x4A, // P2k cmd (0x4A:FSAC)
	0x01, 0x00, // argsize (extra bytes)
	0x00, 0x00, // zero separator
	0x00, 0x00, 0x00, 0x07, // FSAC command
	0x00, '/' , // extra bytes (cmd args)
	0x00, 'a' ,
	0x00, '/' , 
	0xFF, 0xFE,
	0x00, '*' };
	Sendbuf[15]=Vol[0];
	if (strlen((char*)Vol)!=1) { 
		i=0;j=0;
		while (Vol[i]!='*') {
			Sendbuf[j+14]=0;
			Sendbuf[j+15]=Vol[i];
			i++;j=j+2;
		}
		Sendbuf[j+14]=0xFF;
		Sendbuf[j+15]=0xFE;
		j=j+2;
		while (i<strlen((char*)Vol)) {
			Sendbuf[j+14]=0;
			Sendbuf[j+15]=Vol[i];
			i++;j=j+2;
		}
	}
	if (P2k_SendCommand (hdev,Sendbuf,sizeof(Sendbuf),Recbuf)==-1) return 0;
	return(getInt16(Recbuf+14));
}
/*---------------------------------------------------------------------------
 * 
 * name: P2k_ReadSeem
 * @param hdev,num,rec,offs,size,Recbuf
 * @return size 0:error
 */
int P2k_ReadSeem (usb_dev_handle *hdev,int SeemNum,int SeemRec,int SeemOffs,int SeemBytes,unsigned char *Recbuf)
{
	unsigned char Sendbuf[]={
	0xFF, 0xFF, // 2 byte packetID
	0x00, 0x20, // P2k cmd (0x20:Seem read)
	0x00, 0x08, // argsize (extra bytes)
	0x00, 0x00, // zero separator
	0x00, 0x00, // extra bytes (cmd args)
	0x00, 0x00, 
	0x00, 0x00,
	0x00, 0x00};
	setInt16(Sendbuf+8,SeemNum);
	setInt16(Sendbuf+10,SeemRec);
	setInt16(Sendbuf+12,SeemOffs);
	setInt16(Sendbuf+14,SeemBytes);
	return P2k_SendCommand (hdev,Sendbuf,sizeof(Sendbuf),Recbuf);
}
/*---------------------------------------------------------------------------
 * 
 * name: P2k_WriteSeem
 * @param hdev,num,rec,offs,size,Recbuf
 * @return size 0:error
 */
int P2k_WriteSeem (usb_dev_handle *hdev,int SeemNum,int SeemRec,int SeemOffs,int SeemBytes,unsigned char *Recbuf)
{
	unsigned char Sendbuf[0x2710];
	//unsigned char Sendbuf[]={
	//0xFF, 0xFF, // 2 byte packetID
	//0x00, 0x2F, // P2k cmd
	//0x00, 0x08, // argsize (extra bytes)
	//0x00, 0x00, // zero separator
	//0x00, 0x00, // extra bytes (cmd args)
	//0x00, 0x00, 
	//0x00, 0x00,
	//0x00, 0x00};
	setInt16(Sendbuf+0,0xFFFF); // 2 byte packetID
	setInt16(Sendbuf+2,0x002F); // P2k cmd (0x4A:FSAC)
	setInt16(Sendbuf+4,SeemBytes+8); // argsize
	setInt16(Sendbuf+8,SeemNum);
	setInt16(Sendbuf+10,SeemRec);
	setInt16(Sendbuf+12,SeemOffs);
	setInt16(Sendbuf+14,SeemBytes);
	memcpy(Sendbuf+16,Recbuf,SeemBytes);
	return P2k_SendCommand (hdev,Sendbuf,SeemBytes+16,Recbuf);
}
/*---------------------------------------------------------------------------
 * 
 * name: P2k_GetFilelistRecord
 * @param hdev,num,Recbuf
 * @return size 0:error
 */
int P2k_GetFilelistRecord (usb_dev_handle *hdev,int FileNum,unsigned char *Recbuf)
{
	int ret,nFiles;
	//uint16_t nFiles16;
	unsigned char Sendbuf[]={
	0xFF, 0xFF, // 2 byte packetID
	0x00, 0x4A, // P2k cmd
	0x00, 0x05, // argsize (extra bytes)
	0x00, 0x00, // zero separator
	0x00, 0x00, 0x00, 0x08, // FSAC command 
	0x00 };		// number of files in record
	Sendbuf[12]=FileNum;
	ret=P2k_SendCommand (hdev,Sendbuf,sizeof(Sendbuf),Recbuf);
	if (ret<1) return 0;
	nFiles=getInt16(Recbuf+0x0E);
	RecordSize=(ret-4)/nFiles;
	return nFiles;
}
/////////////////////////////////////////////////////////////////////////////
//
// P2k file (FSAC) commands
//
/////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------
 * 
 * name: FSAC_OpenFile
 * @param hdev, filename string (with full path), attrib 
 * @return 
 */
int FSAC_OpenFile(usb_dev_handle *hdev, char *fName, int fAttr)
{
	unsigned char Sendbuf[RecordSize+16];
	//={
	//0xFF, 0xFF, // 2 byte packetID
	//0x00, 0x4A, // P2k cmd (0x4A:FSAC)
	//0x01, 0x00, // argsize (extra bytes) (must fill)
	//0x00, 0x00, // zero separator
	//0x00, 0x00, 0x00, 0x00,		// FSAC command 
	//0x00, 0x00, 0x00, 0x00 };	// extra bytes (cmd args)
	memcpy(Sendbuf+16,fName,strlen(fName));
	setInt16(Sendbuf+0,0xFFFF); // 2 byte packetID
	setInt16(Sendbuf+2,0x004A); // P2k cmd (0x4A:FSAC)
	setInt16(Sendbuf+4,strlen(fName)+8);		// argsize (extra bytes)
	setInt16(Sendbuf+6,0);		// zero separator
	setInt32(Sendbuf+8,0x00000000);// FSAC command 
	setInt32(Sendbuf+12,fAttr);// extra bytes (cmd args) (attrib)
	if (P2k_SendCommand (hdev,Sendbuf,strlen(fName)+16,Recbuf)==-1) return 0;
	return 1;
}
/*---------------------------------------------------------------------------
 * 
 * name: FSAC_DelFile
 * @param hdev, filename string (with full path)
 * @return 
 */
int FSAC_DelFile(usb_dev_handle *hdev,unsigned char *fName)
{
	unsigned char Sendbuf[RecordSize+16];
	//={
	//0xFF, 0xFF, // 2 byte packetID
	//0x00, 0x4A, // P2k cmd (0x4A:FSAC)
	//0x01, 0x00, // argsize (extra bytes) (must fill)
	//0x00, 0x00, // zero separator
	//0x00, 0x00, 0x00, 0x00,		// FSAC command 
	//0x00, 0x00, 0x00, 0x00 };	// extra bytes (cmd args)
	memcpy(Sendbuf+12,fName,strlen((char*)fName));
	setInt16(Sendbuf+0,0xFFFF); // 2 byte packetID
	setInt16(Sendbuf+2,0x004A); // P2k cmd (0x4A:FSAC)
	setInt16(Sendbuf+4,strlen((char*)fName)+4);		// argsize (extra bytes)
	setInt16(Sendbuf+6,0);		// zero separator
	setInt32(Sendbuf+8,0x00000005);// FSAC command 
	if (P2k_SendCommand (hdev,Sendbuf,strlen((char*)fName)+12,Recbuf)==-1) return 0;
	if (isSlave==1) fprintf(stderr,"\n\r");
	return 1;
}
/*---------------------------------------------------------------------------
 * 
 * name: FSAC_CreateFolder
 * @param hdev, filename string (with full path)
 * @return 
 */
int FSAC_CreateFolder(usb_dev_handle *hdev,char *fName)
{
	unsigned char Sendbuf[RecordSize+16];
	//={
	//0xFF, 0xFF, // 2 byte packetID
	//0x00, 0x4A, // P2k cmd (0x4A:FSAC)
	//0x01, 0x00, // argsize (extra bytes) (must fill)
	//0x00, 0x00, // zero separator
	//0x00, 0x00, 0x00, 0x00,		// FSAC command 
	//0x00, 0x00, 0x00, 0x00 };	// extra bytes (cmd args)
	memcpy(Sendbuf+16,fName,strlen(fName));
	setInt16(Sendbuf+0,0xFFFF); // 2 byte packetID
	setInt16(Sendbuf+2,0x004A); // P2k cmd (0x4A:FSAC)
	setInt16(Sendbuf+4,strlen(fName)+8);		// argsize (extra bytes)
	setInt16(Sendbuf+6,0);		// zero separator
	setInt32(Sendbuf+8,0x0000000E); // FSAC command 
	setInt32(Sendbuf+12,0x00000010);// Folder attrib 
	if (P2k_SendCommand (hdev,Sendbuf,strlen(fName)+16,Recbuf)==-1) return 0;
	return 1;
}
/*---------------------------------------------------------------------------
 * 
 * name: FSAC_RemoveFolder
 * @param hdev, filename string (with full path)
 * @return 
 */
int FSAC_RemoveFolder(usb_dev_handle *hdev,char *fName)
{
	unsigned char Sendbuf[RecordSize+16];
	//={
	//0xFF, 0xFF, // 2 byte packetID
	//0x00, 0x4A, // P2k cmd (0x4A:FSAC)
	//0x01, 0x00, // argsize (extra bytes) (must fill)
	//0x00, 0x00, // zero separator
	//0x00, 0x00, 0x00, 0x00,		// FSAC command 
	//0x00, 0x00, 0x00, 0x00 };	// extra bytes (cmd args)
	memcpy(Sendbuf+16,fName,strlen(fName));
	setInt16(Sendbuf+0,0xFFFF); // 2 byte packetID
	setInt16(Sendbuf+2,0x004A); // P2k cmd (0x4A:FSAC)
	setInt16(Sendbuf+4,strlen(fName)+8);		// argsize (extra bytes)
	setInt16(Sendbuf+6,0);		// zero separator
	setInt32(Sendbuf+8,0x0000000F); // FSAC command 
	setInt32(Sendbuf+12,0x00000010);// Folder attrib 
	if (P2k_SendCommand (hdev,Sendbuf,strlen(fName)+16,Recbuf)==-1) return 0;
	return 1;
}
/*---------------------------------------------------------------------------
 * 
 * name: FSAC_CloseFile
 * @param hdev 
 * @return 
 */
int FSAC_CloseFile(usb_dev_handle *hdev)
{
	unsigned char Sendbuf[]={
	0xFF, 0xFF, // 2 byte packetID
	0x00, 0x4A, // P2k cmd (0x4A:FSAC)
	0x00, 0x04, // argsize (extra bytes) (must fill)
	0x00, 0x00, // zero separator
	0x00, 0x00, 0x00, 0x04}; // FSAC command 
	if (P2k_SendCommand (hdev,Sendbuf,sizeof(Sendbuf),Recbuf)==-1) return 0;
	return 1;
}
/*---------------------------------------------------------------------------
 * 
 * name: FSAC_SeekFile
 * @param hdev,offset,direction 
 * @return 
 */
int FSAC_SeekFile(usb_dev_handle *hdev,int offset,char direction)
{
	unsigned char Sendbuf[]={
	0xFF, 0xFF, // 2 byte packetID
	0x00, 0x4A, // P2k cmd (0x4A:FSAC)
	0x00, 0x09, // argsize (extra bytes) (must fill)
	0x00, 0x00, // zero separator
	0x00, 0x00, 0x00, 0x03, // FSAC command 
	0x00, 0x00, 0x00, 0x00, // arg (offset)
	0x00 };
	setInt32(Sendbuf+12,offset);
	Sendbuf[16]=direction;
	if (P2k_SendCommand (hdev,Sendbuf,sizeof(Sendbuf),Recbuf)==-1) return 0;
	return 1;
}
/*---------------------------------------------------------------------------
 * 
 * name: FSAC_ReadFile
 * @param hdev,buffer,size (basic command maxsize:0x400)
 * @return 
 */
int FSAC_ReadFile(usb_dev_handle *hdev,unsigned char *buffer,int size)
{
	int bytes_done=0,chunksize=0x400,i=0,k=0,numchunks;
	unsigned char Sendbuf[]={
	0xFF, 0xFF, // 2 byte packetID
	0x00, 0x4A, // P2k cmd (0x4A:FSAC)
	0x00, 0x08, // argsize (extra bytes) (must fill)
	0x00, 0x00, // zero separator
	0x00, 0x00, 0x00, 0x01, // FSAC command 
	0x00, 0x00, 0x00, 0x00}; // arg (size)
	if (hdev==0) { usb_find_devices(); hdev=P2k_Connect(hdev); }
	if (hdev!=0) {
		numchunks=size/0x400+1;
		if (isSlave==0) fprintf(stderr,"%04u/00000 100",numchunks);
		else fprintf(stderr,"%04u",numchunks);
		while (bytes_done<size) {
			if (size-bytes_done<0x400) chunksize=size-bytes_done;
			setInt16(Sendbuf+14,chunksize);
			//fprintf(stderr,"%04u %04u\n",chunksize,bytes_done);
			if (P2k_SendCommand (hdev,Sendbuf,sizeof(Sendbuf),Recbuf)==-1) return 0;
			memcpy(buffer+bytes_done,Recbuf+14,chunksize);
			bytes_done=bytes_done+chunksize;i=i+1;
			k=i*100/numchunks;
			if (isSlave==0) fprintf(stderr,"\b\b\b\b\b\b\b\b\b%04u %03u%%",i,k);
			else fprintf(stderr,".");  
		}
		if (isSlave==0) fprintf(stderr,"\n");
		else fprintf(stderr,"\n\r");
		return 1;
	}
	return 0;
}
/*---------------------------------------------------------------------------
 * 
 * name: FSAC_WriteFile
 * @param hdev,buffer,size (basic command maxsize:0x400)
 * @return 
 */
int FSAC_WriteFile(usb_dev_handle *hdev,unsigned char *buffer,int size)
{
	int bytes_done=0,chunksize=0x400,i=0,k=0,numchunks;
	unsigned char Sendbuf[size+16];
	//={
	//0xFF, 0xFF, // 2 byte packetID
	//0x00, 0x4A, // P2k cmd (0x4A:FSAC)
	//0x01, 0x00, // argsize (extra bytes) (must fill)
	//0x00, 0x00, // zero separator
	//0x00, 0x00, 0x00, 0x02,		// FSAC command 
	//0x00, 0x00, 0x00, 0x00 };	// extra bytes (cmd args)
	if (hdev==0) { usb_find_devices(); hdev=P2k_Connect(hdev); }
	if (hdev!=0) {
		setInt16(Sendbuf+0,0xFFFF); // 2 byte packetID
		setInt16(Sendbuf+2,0x004A); // P2k cmd (0x4A:FSAC)
		setInt16(Sendbuf+6,0);		// zero separator
		setInt32(Sendbuf+8,0x00000002);// FSAC command 
		numchunks=size/0x400+1;
		if (isSlave==0) fprintf(stderr,"%04u/00000 100",numchunks);
		else fprintf(stderr,"%04u",numchunks);
		while (bytes_done<size) {
			if (size-bytes_done<0x400) chunksize=size-bytes_done;
			setInt16(Sendbuf+4,chunksize+8);	// argsize (extra bytes)
			setInt32(Sendbuf+12,chunksize);		// extra bytes (cmd args)
			memcpy(Sendbuf+16,buffer+bytes_done,chunksize);
			if (P2k_SendCommand (hdev,Sendbuf,chunksize+16,Recbuf)==-1) return 0;
			bytes_done=bytes_done+chunksize;i=i+1;
			k=i*100/numchunks;
			if (isSlave==0) fprintf(stderr,"\b\b\b\b\b\b\b\b\b%04u %03u%%",i,k);
			else fprintf(stderr,".");  
		}
		if (isSlave==0) fprintf(stderr,"\n");
		else fprintf(stderr,"\n\r");
		return 1;
	}
	return 0;
}
/////////////////////////////////////////////////////////////////////////////
//
// P2k functions
//
/////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------
 * 
 * name: P2k_DownloadSeem
 * @param hdev,num,rec,offs,size,Recbuf
 * @return
 */
int P2k_DownloadSeem (usb_dev_handle *hdev,int SeemNum,int SeemRec,char *dest)
{
	int ret;
	FILE *hFile;
	char filename[80];
	//hdev=Find_TC_device();
	if (hdev==0) { usb_find_devices(); hdev=P2k_Connect(hdev); }
	if (hdev!=0) {
		ret=P2k_ReadSeem(hdev,SeemNum,SeemRec,0,0,Recbuf);
		if (ret==0) return 0;
		// write file
		// ret=getInt16(Recbuf+4)-9;
		sprintf(filename,"%s%04x_%04x.seem",dest,SeemNum,SeemRec);
		hFile=fopen(filename,"wb+");
		ret=fwrite(Recbuf+0x0F,1,ret-1,hFile);
		fclose(hFile);
		if (isSlave==1) fprintf(stderr,"\r\n");
		else fprintf(stderr,"\n");
		return ret;
	}
	return 0;
}
/*---------------------------------------------------------------------------
 * 
 * name: P2k_UploadSeem
 * @param hdev,filename
 * @return
 */
int P2k_UploadSeem (usb_dev_handle *hdev, char *filename)
{
	int i,SeemNum,SeemRec,fsize;
	FILE *hFile;
	char *buffer;
	char tmp1[1000],tmp2[1000], filename2[1000];
	char *string_ptr;
	char *string_ptr2;

	//hdev=Find_TC_device();
	if (hdev==0) { usb_find_devices(); hdev=P2k_Connect(hdev); }
	if (hdev!=0) {
		strcpy(filename2,filename);
		while((string_ptr=strpbrk(filename2,"_"))!=NULL) *string_ptr=' ';
		while((string_ptr2=strpbrk(filename2,"."))!=NULL) *string_ptr2=' ';
		sscanf(filename2,"%s %s",tmp1,tmp2);
		string_ptr=strrchr(tmp1,'/');
		SeemRec=htoi(tmp2);
		if (string_ptr==NULL) SeemNum=htoi(tmp1);
		else SeemNum=htoi(++string_ptr);
		fprintf(stderr,"Upload seem: %x %x\n",SeemNum,SeemRec);
		// read file
		hFile=fopen(filename,"rb+");
		if (hFile==NULL) { fprintf(stderr,"!File not found!\n"); return 0;}
		fseek(hFile,0,SEEK_END);
		fsize=ftell(hFile);
		fseek(hFile,0,SEEK_SET);
		// prepare buffer
		buffer = (char*) malloc (fsize);
		fread(buffer,1,fsize,hFile);
		fclose (hFile);
		// upload seem data
		i=P2k_WriteSeem(hdev,SeemNum,SeemRec,0,fsize,(unsigned char*)buffer);
		free (buffer);
		return i;
	}
	return 0;
}
/*---------------------------------------------------------------------------
 * 
 * name: P2k_Restart
 * @param hdev
 * @return
 */
int P2k_Restart (usb_dev_handle *hdev)
{
	unsigned char Sendbuf[]={
	0xFF, 0xFF, // 2 byte packetID
	0x00, 0x22, // P2k cmd
	0x00, 0x00, // argsize (extra bytes)
	0x00, 0x00};// zero separator

	//hdev=Find_TC_device();
	if (hdev==0) { usb_find_devices(); hdev=P2k_Connect(hdev); }
	if (hdev!=0) { return P2k_SendCommand (hdev,Sendbuf,sizeof(Sendbuf),Recbuf); }
	return 0;
}
/*---------------------------------------------------------------------------
 * 
 * name: P2k_Suspend
 * @param hdev
 * @return
 */
int P2k_Suspend (usb_dev_handle *hdev)
{
	unsigned char Sendbuf[]={
	0xFF, 0xFF, // 2 byte packetID
	0x00, 0x36, // P2k cmd
	0x00, 0x01, // argsize (extra bytes)
	0x00, 0x00, // zero separator
	00 };

	//hdev=Find_TC_device();
	if (hdev==0) { usb_find_devices(); hdev=P2k_Connect(hdev); }
	if (hdev!=0) { return P2k_SendCommand (hdev,Sendbuf,sizeof(Sendbuf),Recbuf); }
	return 0;
}
/*---------------------------------------------------------------------------
 * 
 * name: P2k_Flash
 * @param hdev
 * @return
 */
int P2k_Flash (usb_dev_handle *hdev)
{
	unsigned char Sendbuf[]={
	0xFF, 0xFF, // 2 byte packetID
	0x00, 0x0D, // P2k cmd
	0x00, 0x00, // argsize (extra bytes)
	0x00, 0x00};// zero separator

	//hdev=Find_TC_device();
	if (hdev==0) { usb_find_devices(); hdev=P2k_Connect(hdev); }
	if (hdev!=0) { return P2k_SendCommand (hdev,Sendbuf,sizeof(Sendbuf),Recbuf); }
	return 0;
}
/*---------------------------------------------------------------------------
 * 
 * name: P2k_AT
 * @param hdev
 * @return
 */
int P2k_AT (usb_dev_handle *hdev)
{
	if (hdev==0) { usb_find_devices(); hdev=P2k_Connect(hdev); }
	if (hdev!=0) { return Switch_P2ktoATmode (hdev); }
	return 0;
}
/////////////////////////////////////////////////////////////////////////////
//
// Main 
//
/////////////////////////////////////////////////////////////////////////////
/*---------------------------------------------------------------------------
 * 
 * name: s_ Search on command table
 * @param
 * @return  found index, 0:not found
 */
unsigned int s_(char* str, char* cmp[])
{
	unsigned int i = 0;
	while (cmp[i] != 0) {
		if (strcmp(str, cmp[i]) == 0) return i+1;
		i++;
	}
	return 0;
} 
/*---------------------------------------------------------------------------
 * 
 * name: main
 * @param
 * @return  1:found p2k phone, 0:not found
 */
int main(int argc, char *argv[])
{
	int i,j,k,t,ret,SeemNum,SeemRec,filesize;
	char cmd[1000],ArgCmd[1000],tmp1[1000],tmp3[1000],filenameW[1000];
	unsigned char tmp2[1000];
	char * cmdLine;
	char *string_ptr;
	char * bufFileList,* bufFileList2;
	char * ptrFileList;
	char * filebuffer;
	char * filename;
	FILE *hFile;
	usb_dev_handle *hdev = 0; /* the device handle */
	usb_dev_handle *hdev2 = 0; /* the device handle */
	char helpmain[]="V_1.5 Commands: exit,conn,info,seem,file,fold,mode,help,list\nShell commands and command history (up/down arrows) also works.\n---------------------------------------------------------------\n";
	char helpmain2[]="P2k-core V_1.5 activated.\n";
	char helpseem[]="Command format must be: seem d|u SeemNum|Filename SeemRec [Destination folder]\nd:download, u:upload\n";
	char helpfile[]="Command format must be: file d|u|r|a|l [Filename] [Fileattrib|Destination folder]\nd:download, u:upload r:remove a:attrib l:list\n";
	char helpfold[]="Command format must be: fold c|r Foldername\nc:create, r:remove\n";
	char helpmode[]="Command format must be: mode r|s|f|a|m|p|l\nr:restart, s:suspend f:flashmode a:ATmode m:memcard p:p2k l:USBLAN\n";
	char help[]=" P2k-core V_1.5\n \
Commands without any arguments:\n\n \
help    print this.\n \
exit    end application.\n \
conn    try to connect phone.\n \
info    loop thru all volumes and print info\n \
list    list files (before you must got filelist wint 'file l' command)\n\n \
Commands with arguments:\n\n \
info    get some volume info, need to select volume also\n \
        example: info a        select and get info for volume /a\n \
                 info a/mobile/skins/*.ski  with filter string\n \
seem    upload and download seems\n \
        example seem d 117 1    download seem:117, record:1 into the file 0117_0001.seem\n \
                seem u 0117_0001.seem    upload the file as seem:117, record:1\n \
file    list/upload/download/remove files and set attrib\n \
        example:  file d /a/mobile/picture/Some.gif [/home/s5vi/downloaded_files/]\n \
        example:  file u Some.gif /a/mobile/picture/\n \
        example:  file r /a/mobile/picture/Some.gif\n \
        example:  file a /a/mobile/picture/Some.gif 40\n \
        example:  file l\n \
fold    create and remove folders\n \
        example: fold c /a/mobile/skins/newskin        create\n \
                 fold r /a/mobile/skins/newskin        remove\n \
mode    changing phone state\n \
        example:  mode r     restart phone\n \
                  mode s     suspend phone (test mode)\n \
                  mode f     switch phone to flash (bootloader) mode\n \
                  mode a     switch phone to AT (modem) mode\n \
                  mode m     switch phone to Memcard mode\n \
                  mode p     switch phone to P2k mode\n \
                  mode l     switch phone to USBLAN mode\n";
		isDevice=1;Vid=0x22b8;strcpy(ACMdevice,"/dev/ttyACM0");
//
// Get commdandline
//
	for (i=0;i<argc;i++) {
		if (argc > 1 && !strcmp(argv[i], "-v")) isVerbose=1;
		if (argc > 1 && !strcmp(argv[i], "-s")) isSlave=1;
		if (argc > 1 && !strcmp(argv[i], "-d")) isDevice=2;
		if (argc > 1 && !strcmp(argv[i], "-m")) isModem=2;
		if (argc > 1 && !strcmp(argv[i], "-l")) isLAN=1;
		if (isDevice==2 && Vid==0x22b8) { Vid=htoi(argv[i+1]); isDevice=1; }
		if (isModem==2) {
			isModem=1;
			if (argc>i+1) strcpy(ACMdevice,argv[i+1]); else strcpy(ACMdevice,"auto"); 
		} 
	}
	//printf("%04x\n",Vid);return;
//
// Initialize libusb
//
	usb_init(); /* initialize the library */
	usb_find_busses(); /* find all busses */
	usb_find_devices(); /* find all connected devices */

	bufFileList=0;
	if (isSlave==0) fprintf(stderr,helpmain);
	else fprintf(stderr,helpmain2);
	LastVol[0]='a';ptrFileList=bufFileList;
	if (isLAN==0) {
		hdev=P2k_Connect(hdev);
		//
		// get info
		//
		if (hdev!=0) {
						ret=P2k_ReadSeem(hdev,0x117,1,0,0,Recbuf);
						i=0x10;j=0;
						while (Recbuf[i]!=0) {
							tmp2[j++]=Recbuf[i];
							i+=2;
						}
						tmp2[j++]=0;
						fprintf(stderr," Phone model: %s\n",tmp2);
						if (isSlave==1) fprintf(stdout," Connected, phone model: %s\n",tmp2);
			  P2k_GetVolnames(hdev);
							// Get info for all volumes
							j=1; tmp2[1]=0;
							LastVol[1]=0;
							while (VolumeList[j]!=0) {
								tmp2[0]=VolumeList[j];
								i=P2k_GetFreespace (hdev,tmp2);
								FilesNumber=P2k_GetFilesNumber(hdev,tmp2);
								LastVol[0]=tmp2[0];
								fprintf(stderr," Volume: /%s  Free space: %u bytes, %u files found.\n",tmp2,i,FilesNumber);
								j=j+3;
							}
		}
	}
	else {
		hdev=P2k_ConnectLAN(hdev);
		if (hdev!=0) {
			system("ipconfig /all");
		}
	}
	read_history("p2k-core.history");
//
// Infinite cmd loop
//
  char* cmds[] = {"exit", "conn", "info","seem","file","fold","mode","list","help", 0};
	while (1) {
		if (isSlave==0) cmdLine = readline("P2k:> ");
		else cmdLine = readline("");
		add_history(cmdLine);
		strcpy(cmd,cmdLine);
		free(cmdLine);
		unsigned int id = s_(cmd, cmds);
		switch (id) {
			// exit
			case 1:
				fprintf(stderr,"Bye...\n");
				write_history("p2k-core.history");  
				if (hdev>0) { 
					//usb_release_interface(hdev, 1);
					usb_close(hdev);
					if (bufFileList!=0) free(bufFileList);
				}
				return 1;
			// conn
			case 2: usb_find_devices(); hdev=P2k_Connect(hdev); break; 
			// info
			case 3:
				//hdev=Find_TC_device();
				if (hdev==0) { usb_find_devices(); hdev=P2k_Connect(hdev); }
				if (hdev>0) {
					// Phone model ???
					ret=P2k_ReadSeem(hdev,0x117,1,0,0,Recbuf);
					i=0x10;j=0;
					while (Recbuf[i]!=0) {
						tmp2[j++]=Recbuf[i];
						i+=2;
					}
					tmp2[j++]=0;
					fprintf(stderr," Phone model: %s\n",tmp2);
					if (isSlave==1) fprintf(stdout," Connected, phone model: %s\n",tmp2);
					// Get Volume names
					if (P2k_GetVolnames(hdev)==0) return 0;
					if (isVerbose==1) fprintf(stderr," Received volume list: %s\n",VolumeList);
					i=sscanf(cmd,"info %s",ArgCmd);
					if (i==-1) {
						// Get info for all volumes
						j=1; tmp2[1]=0;
						LastVol[1]=0;
						while (VolumeList[j]!=0) {
							tmp2[0]=VolumeList[j];
							i=P2k_GetFreespace (hdev,tmp2);
							FilesNumber=P2k_GetFilesNumber(hdev,tmp2);
							LastVol[0]=tmp2[0];
							fprintf(stderr," Volume: /%s  Free space: %u bytes, %u files found.\n",tmp2,i,FilesNumber);
							j=j+3;
						}
					}
					else {
						// get info for 1 volume
						i=P2k_GetFreespace (hdev,(unsigned char*)ArgCmd);
						FilesNumber=P2k_GetFilesNumber(hdev,(unsigned char*)ArgCmd);
						strcpy(LastVol,ArgCmd);
						//LastVol[0]=ArgCmd[0];
						fprintf(stderr," Volume: /%s  Free space: %u bytes, %u files found.\n",ArgCmd,i,FilesNumber);
					}
				}
				break;
			// seem
			case 4:
				// read args
				i=sscanf(cmd,"seem %s",ArgCmd);
				if (i!=1) { fprintf(stderr,helpseem); break;}
				if (hdev==0) { usb_find_devices(); hdev=P2k_Connect(hdev); }
				if (hdev>0) {
					sscanf(cmd,"seem %s %s %s %s",ArgCmd,tmp1,tmp2,tmp3);
					switch (ArgCmd[0]) {
						case 'd': 
							SeemNum=htoi(tmp1); SeemRec=htoi((char*)tmp2);
							// download seem to file
							fprintf(stderr,"Download:%s%04x_%04x.seem",tmp3,SeemNum,SeemRec);
							P2k_DownloadSeem(hdev,SeemNum,SeemRec,tmp3);
							break;
						case 'u':
							// upload seem from file
							P2k_UploadSeem(hdev,tmp1);
							break;
						default: fprintf(stderr,helpseem);
					}
				}
				break;
			// file
			case 5:
				// read args
				i=sscanf(cmd,"file %s",ArgCmd);
				if (i!=1) { fprintf(stderr,helpfile); break;}
				if (hdev==0) { usb_find_devices(); hdev=P2k_Connect(hdev); }
				if (hdev>0) {
					j=sscanf(cmd,"file %s %s %s",ArgCmd,tmp1,tmp2);
					switch (ArgCmd[0]) {
						case 'd':
							// download file
							if (RecordSize==0) {
								// if Filelist not loaded at all
								strcpy((char*)tmp2,"b/*");
								string_ptr=strpbrk(tmp1,".");
								if (string_ptr==NULL) break;
								strcat((char*)tmp2,string_ptr);
								tmp2[0]=tmp1[1];
								fprintf(stderr,"Filelist not loaded. Get it with %s filter.\n",tmp2);
								FilesNumber=P2k_GetFilesNumber(hdev,tmp2);
								strcpy(LastVol,(char*)tmp2);
								fprintf(stderr," Volume: /%s   %u files found.\n",tmp2,FilesNumber);
								// get filelist
								if (isSlave==0) fprintf(stderr,"%04u/00000 000",FilesNumber);
								else fprintf(stderr,"%04u",FilesNumber);  
								i=1;
								FilesNumber=P2k_GetFilesNumber(hdev,(unsigned char*)LastVol);
								ret=P2k_GetFilelistRecord(hdev,3,Recbuf);
								if (bufFileList!=0) free(bufFileList);
								bufFileList=(char *) malloc(RecordSize*(FilesNumber+2));
								ptrFileList=bufFileList;
								StayQuiet=1;
								while (ret!=0) {
									j=0x12;k=0;
									// fill filelist buffer
									memcpy (ptrFileList,Recbuf+j,ret*RecordSize);
									ptrFileList=ptrFileList+ret*RecordSize;
									while (ret!=0) {
										k=i*100/FilesNumber;
										if (isSlave==0) fprintf(stderr,"\b\b\b\b\b\b\b\b\b%04u %03u%%",i,k);
										else fprintf(stderr,".");  
										j=j+RecordSize;ret--;i++;
									}
									ret=P2k_GetFilelistRecord(hdev,3,Recbuf);
								}
								StayQuiet=0;
								if (isSlave==0) fprintf(stderr,"\nAt Ow Filesize Filename with path\n");
								else fprintf(stderr,"\n");  
								i=1;
								j=0;
								while (bufFileList+j<ptrFileList) {
									fprintf(stderr,"%02x %02x %08u %s\n",
										bufFileList[j+RecordSize-7],
										bufFileList[j+RecordSize-5],
										getInt32((uint8_t*)bufFileList+j+RecordSize-4),
										bufFileList+j);
									j=j+RecordSize;
								}
							}
							// search filename in bufFileList (get attrib and filesize)
							i=0;
							while (i<RecordSize*FilesNumber) {
								if (strcmp(tmp1,bufFileList+i)==0) break; //found
								i=i+RecordSize;
							}
							filesize=getInt32((uint8_t*)bufFileList+i+RecordSize-4);
							fprintf(stderr,"Reading %s %02x %08u bytes, chunks:",bufFileList+i,bufFileList[i+RecordSize-7],filesize);
							filebuffer=(char *) malloc(filesize);
							i=FSAC_OpenFile(hdev,tmp1,bufFileList[i+RecordSize-7]);
							if (i==0) { fprintf (stderr,"!error: opening file!\n"); break; }
							i=FSAC_SeekFile(hdev,0,0);
							if (i==0) fprintf (stderr,"!error: seeking file!\n"); //i=FSAC_CloseFile(hdev); break; }
							i=FSAC_ReadFile(hdev,(unsigned char*)filebuffer,filesize);
							if (i==0) { fprintf (stderr,"!error: reading file!\n"); i=FSAC_CloseFile(hdev); break; }
							i=FSAC_CloseFile(hdev);
							if (i==0) { fprintf (stderr,"!error: closing file!\n"); break; } 
							// prepare filename tmp1->filename
							filename=strrchr(tmp1,'/');
							// write to hdd
							if (j==2) strcpy((char*)tmp2,"."); // if dest folder omitted
							strcat((char*)tmp2,filename);
							//fprintf(stderr,"%s\n",tmp2);
							hFile=fopen((char*)tmp2,"wb+");
							fwrite(filebuffer,1,filesize,hFile);
							fclose(hFile);
							//showhex("file",filebuffer,27);
							free(filebuffer);
							break;
						case 'u':
							// upload file
							// get filesize
							hFile=fopen(tmp1,"rb+");
							if (hFile==NULL) { fprintf(stderr,"!File not found!\n"); break;}
							fseek(hFile,0,SEEK_END);
							filesize=ftell(hFile);
							fseek(hFile,0,SEEK_SET);
							filebuffer = (char*) malloc (filesize);
							// read file
							fread(filebuffer,1,filesize,hFile);
							fclose(hFile);
							// make full target string
							filenameW[0]=0;
							strcpy(filenameW,(char*)tmp2);
							filename=strrchr(tmp1,'/');
							if (filename!=NULL){ filename++;strcat(filenameW,filename);}
							else strcat(filenameW,tmp1);
							fprintf(stderr,"Writing %s %08u bytes, chunks:",filenameW,filesize);
							//fprintf(stderr,"%s\n",filenameW);
							i=FSAC_OpenFile(hdev,filenameW,0);
							if (i==0) { fprintf (stderr,"!error: creating file!\n"); break; }
							i=FSAC_WriteFile(hdev,(unsigned char*)filebuffer,filesize);
							if (i==0) { fprintf (stderr,"!error: writing file!\n"); break; }
							i=FSAC_CloseFile(hdev);
							if (i==0) { fprintf (stderr,"!error: closing file!\n"); break; } 
							free(filebuffer);
							// actualize filelist mem image
							bufFileList2=(char *) malloc(RecordSize*(FilesNumber+2));
							ptrFileList=bufFileList2+RecordSize*FilesNumber;
							memcpy(bufFileList2,bufFileList,RecordSize*FilesNumber);
							free(bufFileList);
							bufFileList=bufFileList2;
							FilesNumber++;
							strcpy(ptrFileList,filenameW);
							setInt32((uint8_t*)ptrFileList+RecordSize-4,filesize);
							ptrFileList[RecordSize-7]=0;
							ptrFileList[RecordSize-5]=7;
							ptrFileList=ptrFileList+RecordSize;
							break;
						case 'r': 
							// remove file
							i=FSAC_DelFile(hdev,(uint8_t*)tmp1);
							if (i==0) { fprintf (stderr,"!error: removing file!\n"); break; } 
							fprintf(stderr,"Removing %s\n",tmp1);
							break;
						case 'a': 
							// set attribs
							i=FSAC_OpenFile(hdev,tmp1,htoi((char*)tmp2));
							if (i==0) { fprintf (stderr,"!error: opening file!\n"); break; } 
							i=FSAC_CloseFile(hdev);
							if (i==0) { fprintf (stderr,"!error: closing file!\n"); break; } 
							break;
						case 'l': 
							// list files
							if (LastVol[0]!=0) {
								sprintf(filenameW,"%s/P2KFILELIST",strdup(getenv("HOME")));
  								//hFile=fopen(strcat(filename,"/P2KFILELIST"),"wb+");
  								hFile=fopen(filenameW,"wb+");
								if (isSlave==0)fprintf(stderr,"%04u/00000 000",FilesNumber);
								else fprintf(stderr,"%04u",FilesNumber); 
								i=1;
								FilesNumber=P2k_GetFilesNumber(hdev,LastVol);
								ret=P2k_GetFilelistRecord(hdev,3,Recbuf);
								if (bufFileList!=0) free(bufFileList);
								bufFileList=(char *) malloc(RecordSize*(FilesNumber+2));
								ptrFileList=bufFileList;
								StayQuiet=1;
								while (ret!=0) {
									j=0x12;k=0;
									// fill filelist buffer
									memcpy (ptrFileList,Recbuf+j,ret*RecordSize);
									ptrFileList=ptrFileList+ret*RecordSize;
									while (ret!=0) {
										k=i*100/FilesNumber;
										if (isSlave==0) fprintf(stderr,"\b\b\b\b\b\b\b\b\b%04u %03u%%",i,k);
										else fprintf(stderr,".");  
										j=j+RecordSize;ret--;i++;
									}
									ret=P2k_GetFilelistRecord(hdev,3,Recbuf);
								}
								StayQuiet=0;
								if (isSlave==0) fprintf(stderr,"\nAt Ow Filesize Filename with path\n");
								//else fprintf(stderr,"\n");  
								i=1;
								j=0;
								while (bufFileList+j<ptrFileList) {
									if (isSlave==0) fprintf(stderr,"%02x %02x %08u %s\n",bufFileList[j+RecordSize-7],bufFileList[j+RecordSize-5],getInt32((uint8_t*)bufFileList+j+RecordSize-4),bufFileList+j);
									else fprintf(hFile,"%s\n%u\n%02x %02x\n",bufFileList+j,getInt32((uint8_t*)bufFileList+j+RecordSize-4),bufFileList[j+RecordSize-7],bufFileList[j+RecordSize-5]);
									j=j+RecordSize;
								}
							fclose(hFile);
							if (isSlave==0) fprintf(stderr,"\n");  
							else fprintf(stderr,"\n\r");
							}
							else printf ("Firstly pls select a volume with info cmd.\n");
							//showhex("table",bufFileList,RecordSize*(20));
							break;
						default: fprintf(stderr,helpfile);
					}
				}
				break;
			// folder
			case 6: 
				// read args
				i=sscanf(cmd,"fold %s",ArgCmd);
				if (i!=1) { fprintf(stderr,helpfold); break;}
				if (hdev==0) { usb_find_devices(); hdev=P2k_Connect(hdev); }
				if (hdev>0) {
					sscanf(cmd,"fold %s %s",ArgCmd,tmp1);
					switch (ArgCmd[0]) {
						case 'c':
							// create folder
							i=FSAC_CreateFolder(hdev,tmp1);
							if (i==0) { fprintf (stderr,"!error: creating folder!\n"); break; }
							fprintf(stderr,"Creating %s\n",tmp1);
							if (isSlave==1) fprintf(stderr,"\r");
							break;
						case 'r':
							// remove folder
							i=FSAC_RemoveFolder(hdev,tmp1);
							if (i==0) fprintf (stderr,"!error: removing folder!\n");
							fprintf(stderr,"Removing %s\n",tmp1);
							break;
						default: fprintf(stderr,helpfold);
					}
				}
				break;
			// mode
			case 7:
				// read args
				if (hdev==0) { usb_find_devices(); hdev=P2k_Connect(hdev); }
				if (hdev>0) {
					i=sscanf(cmd,"mode %s %s",ArgCmd,LANcommand);
					if (i<1) { fprintf(stderr,helpmode); break;}
					switch (ArgCmd[0]) {
						case 'r':
							// restart phone
							P2k_Restart(hdev);
							fprintf(stderr,"You must be reconnect when phone restarted.\n");
							break;
						case 's':
							// switch to suspend mode
							P2k_Suspend(hdev);
							fprintf(stderr,"Phone suspended. To wakeup you can restart it.\n");
							break;
						case 'f':
							// switch to flashmode
							P2k_Flash(hdev);
							fprintf(stderr,"Exiting, phone is in flash mode.\n");
							return 1;
						case 'a':
							// switch to ATmode
							P2k_AT(hdev);
							t=time(NULL);hdev=NULL;
							if (isVerbose==1) isVerbose=2;
							// loop, waiting to AT device appear
							fprintf(stderr," Search for Motorola Data Interface.\n");
							while (time(NULL)-t<5) {
								usb_find_devices(); /* find all connected devices */
								hdev2=open_dev("Motorola Data Interface");
								if (hdev2!=0) break;
								usleep(1000);
							}
							if (isVerbose==2) isVerbose=1;
							break;
						case 'm':
							// switch to Memcard mode
							P2k_AT(hdev);
							t=time(NULL);hdev=NULL;
							if (isVerbose==1) isVerbose=2;
							// loop, waiting to AT device appear
							fprintf(stderr," Search for Motorola Data Interface.\n");
							while (time(NULL)-t<5) {
								usb_find_devices(); /* find all connected devices */
								hdev=Find_AT_device();
								if (hdev!=0) break;
								usleep(1000);
							}
							if (isVerbose==2) isVerbose=1;
							if (hdev!=0) {
								// get AT device info and switch
								ATep_out=dev->config[nC].interface[nI].altsetting[nA].endpoint[0].bEndpointAddress;
								// check endpoint order
								if (ATep_out<0x80) ATep_in=dev->config[nC].interface[nI].altsetting[nA].endpoint[1].bEndpointAddress;
								else {
									ATep_out=dev->config[nC].interface[nI].altsetting[nA].endpoint[1].bEndpointAddress;
									ATep_in=dev->config[nC].interface[nI].altsetting[nA].endpoint[0].bEndpointAddress;
								}
								AT_ifnum=dev->config[nC].interface[nI].altsetting[nA].bInterfaceNumber;
								fprintf (stderr," Inteface number: %02x, Endpoints: %02x %02x\n",AT_ifnum,ATep_in,ATep_out);
								if(usb_claim_interface(hdev, AT_ifnum) < 0) {
									fprintf(stderr,"!error: claiming interface 1 failed\n");
									usb_close(hdev);
									return 0;
								}
							}
							Switch_ATtoMemmode(hdev);
							sleep(1);
							t=time(NULL);hdev=NULL;
							if (isVerbose==1) isVerbose=2;
							// loop, waiting to MEM device appear
							fprintf(stderr," Search for Motorola Mass Storage Interface.\n");
							while (time(NULL)-t<5) {
								usb_find_devices(); /* find all connected devices */
								hdev=open_dev("Motorola Mass Storage Interface.");
								if (hdev!=0) break;
								usleep(1000);
							}
							if (isVerbose==2) isVerbose=1;
							break;
						case 'p':
							usb_find_devices(); hdev=P2k_Connect(hdev);
							break;
						case 'l':
							usb_find_devices(); hdev=P2k_ConnectLAN(hdev);
							break;
						default: fprintf(stderr,helpmode);
					}
				}
				break;
			// list files
			case 8:
				if (ptrFileList==bufFileList) printf ("Please read filelist first with 'file l' command.\n");
				fprintf(stderr,"At Ow Filesize Filename with path\n");i=1;
				j=0;
				while (bufFileList+j<ptrFileList) {
					fprintf(stderr,"%02x %02x %08u %s\n",bufFileList[j+RecordSize-7],bufFileList[j+RecordSize-5],getInt32((uint8_t*)bufFileList+j+RecordSize-4),bufFileList+j);
					j=j+RecordSize;
				}
				break;
			// help
			case 9:
				fprintf(stderr,help);
				//system ("cat help.txt");
				break;
			default: system (cmd);
		}
	}
	return 1;
}
