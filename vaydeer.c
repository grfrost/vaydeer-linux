/*
 * Allow vaydeer keypads to be used on linux platforms. 
 *
 * Copyright (c) 2025 Gary Frost
 *
 * The code may be used by anyone for any purpose,
 * and can serve as a starting point for developing
 * applications using hidraw.
 */


#include <libudev.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {

 // We are going to be scanning this dir
   const char *sysClassHidrawDirName = "/sys/class/hidraw";
   DIR *sysClassHidrawDir;
   if((sysClassHidrawDir = opendir(sysClassHidrawDirName)) == NULL) {
      fprintf(stderr,"cannot open directory: %s\n", sysClassHidrawDirName);
      exit(1);
   }

   chdir(sysClassHidrawDirName);
   char fullpath[PATH_MAX + 1]; 
   int fdCount = 0;
   struct pollfd pfds[20]; /// we only expect to find 20 keys

   struct dirent *entry;
   while((entry = readdir(sysClassHidrawDir)) != NULL) {
      realpath(entry->d_name, fullpath);
      struct stat statbuf;
      lstat(entry->d_name, &statbuf);
      if(S_ISLNK(statbuf.st_mode)) {

         struct udev *udev = udev_new();
         if (!udev) {
            fprintf(stderr, "Cannot create udev context.\n");
            exit(1);
         }

         struct udev_device *dev_child = udev_device_new_from_syspath(udev, fullpath);
         if (!dev_child) {
            fprintf(stderr, "Failed to get device.\n");
            exit(1);
         }

         struct udev_device *dev_parent = udev_device_get_parent(dev_child);
         if (!dev_parent){
            fprintf(stderr, "Failed to get parent of device.\n");
            exit(1);
         }else{
            const char *hidName = udev_device_get_property_value(dev_parent, "HID_NAME");
            if (strncmp("Vaydeer", hidName, 5)==0){
               const char *hidPhys = udev_device_get_property_value(dev_parent, "HID_PHYS");
               const char *devNode = udev_device_get_devnode(dev_child);
               int fd = open(devNode, O_RDONLY|O_NONBLOCK);
               if (fd >=0){
                  //fprintf(stdout, "Opened Vaydeer %s + %s \n", devNode, hidPhys);
                  pfds[fdCount].fd = fd;
                  pfds[fdCount].events = POLLIN;
                  fdCount++;
               }else{
                  fprintf(stderr, " failed to open %s did you forget to run using sudo\n", devNode);
                  exit(1);
               }
            }

         }

         /* free dev */
         udev_device_unref(dev_child);

         /* free udev */
         udev_unref(udev);
      }
   }
   closedir(sysClassHidrawDir); 
   if (fdCount > 0){
      fprintf(stdout, "Scanning %d Vaydeer hidraw devices whilst this process is running \n",fdCount);
      char* tumbler = "-\\|-/";
      char *ptr = tumbler;

      // We should have the fd's for the Vaydeer devices
      int preturn=0;
      while ((preturn = poll(pfds,fdCount,1000)) >=0){
         for (int i=0; i< fdCount; i++){
            if (pfds[i].revents & POLLIN){
               char ch;
               int count =  read(pfds[i].fd, &ch, 1);
               fprintf(stdout, "%c\b", *ptr);
               ptr++;
               if (*ptr=='\0'){
                 ptr = tumbler;
               }
               fflush(stdout);
            }
         }
      }
      fprintf(stdout, "poll returned %d\n", preturn);
   }else{
      fprintf(stderr, "Failed to find any Vaydeer keyboards \n");
      exit(1);
   }
   return 0;
}


