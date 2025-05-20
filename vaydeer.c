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
#include <sys/file.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

/// we only expect to find 20 keys
#define MAX_KEYS 20 

int main(int argc, char **argv) {
   int tumble = 0;
   int verbose = 0;
   int help = 0;
   int info = 0;
   int key = 0;
   int pid = 0;
   int bad = 0;
   char*lock=NULL;
   for (int i=1; i<argc; i++){
      if (((strcmp(argv[i], "--lock")==0) || (strcmp(argv[i], "-l")==0)) && ((i+1)<argc)){
         i++;
         lock = argv[i];
      }else if ((strcmp(argv[i], "--pid")==0) || (strcmp(argv[i], "-p")==0)){
         pid =1;
      }else if ((strcmp(argv[i], "--info")==0) || (strcmp(argv[i], "-i")==0)){
         info=1;
      }else if ((strcmp(argv[i], "--tumble")==0) || (strcmp(argv[i], "-t")==0)){
         tumble=1;
      }else if ((strcmp(argv[i], "--key")==0) || (strcmp(argv[i], "-k")==0)){
         key=1;
      }else if ((strcmp(argv[i], "--verbose")==0) || (strcmp(argv[i], "-v")==0)){
         verbose=1;
      } else if ((strcmp(argv[i], "--help")==0) || (strcmp(argv[i], "-h")==0)){
         help=1;
      } else{
         fprintf(stderr, "%s ?\n", argv[i]);
         help=1;
         bad = 1;
      }
   }
   if (bad){
      fprintf(stderr, "Bailed...\n");
      exit(1);
   }
   
   int mypid = getpid();

   if (pid){
      fprintf(stdout, "%d\n", mypid);
   }
   if (lock){
      if (info){
         fprintf(stdout, "Would try to lock %s\n", lock);
      }else{
         if (verbose){
              fprintf(stdout, "Trying to aquire lock %s\n", lock);
         }
         int lockFd = open(lock, O_RDWR|O_EXCL|O_CREAT, 0666);
         if (verbose){
              fprintf(stdout, "Lock fd   %d\n", lockFd);
         }
         if (lockFd <0){
             fprintf(stderr, "lock exists... is vaydeer already running ? Bailed \n");
             exit(1);
         }else{
             char buf[100];
             int count = sprintf(buf, "%d", mypid);
             write(lockFd, buf, count);
             close(lockFd);
         }
      }
   }

   const char *sysClassHidrawDirName = "/sys/class/hidraw";
   if (verbose){
      fprintf(stdout, "We are going to be scanning %s\n",sysClassHidrawDirName);
   }
   DIR *sysClassHidrawDir;
   if((sysClassHidrawDir = opendir(sysClassHidrawDirName)) == NULL) {
      fprintf(stderr,"cannot open directory: %s\n", sysClassHidrawDirName);
      exit(1);
   }

   chdir(sysClassHidrawDirName);
   char fullpath[PATH_MAX + 1]; 
   int fdCount = 0;
   struct pollfd pfds[MAX_KEYS];

   struct dirent *entry;
   while((entry = readdir(sysClassHidrawDir)) != NULL) {
      realpath(entry->d_name, fullpath);
      if (verbose){
        fprintf(stdout, "checking %s\n", entry->d_name);
      }
      struct stat statbuf;
      lstat(entry->d_name, &statbuf);

      if(S_ISDIR(statbuf.st_mode)) {
         if (verbose){
           fprintf(stdout, "    %s is a dir skipping \n", entry->d_name);
         }
      }else if(S_ISLNK(statbuf.st_mode)) {
         if (verbose){
           fprintf(stdout, "    %s is a link \n", entry->d_name);
         }

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
         if (verbose){
            fprintf(stdout, "    Got udev access %s to  \n", entry->d_name);
         }

         struct udev_device *dev_parent = udev_device_get_parent(dev_child);
         if (!dev_parent){
            fprintf(stderr, "Failed to get parent of device.\n");
            exit(1);
         }else{
            const char *hidName = udev_device_get_property_value(dev_parent, "HID_NAME");
            if (verbose){
               fprintf(stdout, "    Got udev access to %s's parent \n", entry->d_name);
               fprintf(stdout, "       Parent's HID_NAME=%s\n", hidName);
            }
            
            if (strncmp("Vaydeer", hidName, 5)==0){
               if (verbose){
                  fprintf(stdout, "       Is a Vaydeer device=\n");
               }
               const char *hidPhys = udev_device_get_property_value(dev_parent, "HID_PHYS");
               const char *devNode = udev_device_get_devnode(dev_child);
               int fd = open(devNode, O_RDONLY|O_NONBLOCK);
               if (fd >=0){
                  //fprintf(stdout, "Opened Vaydeer %s + %s \n", devNode, hidPhys);
                  pfds[fdCount].fd = fd;
                  pfds[fdCount].events = POLLIN;
                  if (verbose){
                      fprintf(stdout, "       Opened as fd %d and added to fd[%d]\n", fd, fdCount);
                  }
                  fdCount++;
               }else{
                  fprintf(stderr, " failed to open %s did you forget to run using sudo\n", devNode);
                  exit(1);
               }
            }else{
               if (verbose){
                  fprintf(stdout, "       Skipped \n");
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
      if (!info){
         if (verbose){
            fprintf(stdout, "About to start scanning %d Vaydeer devices  \n",fdCount);
         }
         char* tumbler = "-\\|-/";
         char *ptr = tumbler;
   
         // We should have the fd's for the Vaydeer devices
         int preturn=0;
         while ((preturn = poll(pfds,fdCount,1000)) >=0){
            for (int i=0; i< fdCount; i++){
               if (pfds[i].revents & POLLIN){
                  char ch;
                  int count =  read(pfds[i].fd, &ch, 1);
                  if (tumble){
                     fprintf(stdout, "%c\b", *ptr);
                     ptr++;
                     if (*ptr=='\0'){
                        ptr = tumbler;
                     }
                     fflush(stdout);
                  }
                  if (key){
                     fprintf(stdout, "%d ", i);
                     fflush(stdout);
                  }
               }
            }
         }
         if (verbose){
            fprintf(stdout, "poll returned %d\n", preturn);
         }
         for (int i=0; i<fdCount; i++){
            close(pfds[i].fd);
         }
     }else{
         fprintf(stdout, "Would scan %d Vaydeer devices  \n",fdCount);
         for (int i=0; i<fdCount; i++){
            close(pfds[i].fd);
         }
     }
  }else{
     fprintf(stderr, "Failed to find any Vaydeer keyboards \n");
     exit(1);
   }
   return 0;
}


