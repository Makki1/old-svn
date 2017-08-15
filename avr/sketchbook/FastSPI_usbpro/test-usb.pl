#!/usr/bin/perl

use strict;
use Time::HiRes qw ( time alarm sleep usleep gettimeofday ); # uS resolution

#DON'T use Device::SerialPort; its bloatware and does many things we don't want
#use POSIX qw(:termios_h); # Used terminal IO library
my $dev = "/dev/usbserial-6493534343335180D060";
#$dev = "/dev/ttyACM1";
# no buffers, no fancy shit, just do it
`stty -F /dev/usbserial-6493534343335180D060 115200 cs8`;
# -raw -nohup

open(PORT,"+> $dev") or die("failed $ \n");
# make unbuffered under any circumstances, preserve old FH (STDOUT)
my $oldh = select(PORT);
$| = 1;
select($oldh);

# Arduino defaults to reset on opening port (DTR toggled) so wait a little
sleep(3);
my $demo=0;
my $res;
my $som = 0x7E;
my $eom = 0xE7;
my $label;
my $message;
my $lenMSB=0;
my $lenLSB=1;
my @data = ();

binmode PORT;

#TODO: for UNIT test all labels



 
### Unit 1 ###
# test usbpro-messages
$label = 0x03;
@data = (0x50, 0x51, 0x52);
$lenMSB = scalar(@data) >> 8;
$lenLSB = scalar(@data) & 0xFF;
print "test usbpro label $label, data @data, lenLSB $lenLSB, lenMSB $lenMSB\n";
$message = pack("C*", ($som, $label, $lenLSB, $lenMSB));
for (my $i = 0; $i < (($lenMSB << 8) + $lenLSB); $i++) {
    $message .= pack("C*", $data[$i]);
}
$message .= pack("C*", ($eom));
syswrite PORT,$message;
sleep(0.05); # 50ms


### Unit 2 ###
### test usbpro-message64
$label = 0x06;
for (my $j=0;$j<64;$j++) {
    $data[$j] = 0x64+$j;
}
$data[0]=0;
$lenMSB = scalar(@data) >> 8;
$lenLSB = scalar(@data) & 0xFF;
print "test usbpro label $label, data @data, lenLSB $lenLSB, lenMSB $lenMSB\n";
$message = pack("C*", ($som, $label, $lenLSB, $lenMSB));
for (my $i = 0; $i < (($lenMSB << 8) + $lenLSB); $i++) {
    $message .= pack("C*", ($data[$i]));
}
$message .= pack("C*", ($eom));
#syswrite PORT,$message;
sleep(0.5); # 50ms

### Unit 2 off ###
### test usbpro-message64
$label = 0x06;
for (my $j=0;$j<64;$j++) {
    $data[$j] = 0;
}
$data[0]=0;
$lenMSB = scalar(@data) >> 8;
$lenLSB = scalar(@data) & 0xFF;
print "test usbpro label $label, data @data, lenLSB $lenLSB, lenMSB $lenMSB\n";
$message = pack("C*", ($som, $label, $lenLSB, $lenMSB));
for (my $i = 0; $i < (($lenMSB << 8) + $lenLSB); $i++) {
    $message .= pack("C*", ($data[$i]));
}
$message .= pack("C*", ($eom));
#syswrite PORT,$message;

### Unit READ ###
print "reading result: ";
sysread PORT,$res,255;
my  $bufhex = $res;
$bufhex =~ s/(.)/sprintf("0x%x ",ord($1))/eg;
print "READ: $bufhex \n\n";

### Unit 3 ###
### test RDM
# Label, Size:  0x52 26 D:0: 0xCC  D:1: 0x1  D:2: 0x18  D:3: 0x7A  D:4: 0x70  D:5: 0x0  D:6: 0x0  D:7: 0x1  D:8: 0x23  D:9: 0x7A  D:10: 0x70  D:11: 0x8C  D:12: 0x3  D:13: 0x11  D:14: 0xAC  D:15: 0x0  D:16: 0x1  D:17: 0x0  D:18: 0x0  D:19: 0x0  D:20: 0x20  D:21: 0x0  D:22: 0x81  D:23: 0x0  D:24: 0x4  D:25: 0xCB 
# Label, Size:  0x52 26 D:0: 0xCC  D:1: 0x1  D:2: 0x18  D:3: 0x7A  D:4: 0x70  D:5: 0x0  D:6: 0x0  D:7: 0x1  D:8: 0x23  D:9: 0x7A  D:10: 0x70  D:11: 0x8C  D:12: 0x3  D:13: 0x11  D:14: 0xAC  D:15: 0x1  D:16: 0x1  D:17: 0x0  D:18: 0x0  D:19: 0x0  D:20: 0x20  D:21: 0x0  D:22: 0x82  D:23: 0x0  D:24: 0x4  D:25: 0xCD 
# 0xCC  0x1  0x18  0x7A  0x70  0x1  0xDE  0x49  0x9F  0x7A  0x70  0x8C  0x3  0x11  0xAC  0x0  0x1  0x0  0x0  0x0  0x20  0x0  0x81  0x0  0x6  0x6E 

#@data = (0xcc, 0x1, 0x18, 0x7a, 0x70, 0x0, 0x0, 0x1, 0x23, 0x7a, 0x70, 0x8c, 0x3, 0x11, 0xac, 0x0, 0x1, 0x0, 0x0, 0x0, 0x20, 0x0, 0x81, 0x0, 0x4, 0xcb);
@data = (0xcc, 0x1, 0x18, 0x7a, 0x70, 0x1, 0xDE, 0x49, 0x9F, 0x7a, 0x70, 0x8c, 0x3, 0x11, 0xac, 0x0, 0x1, 0x0, 0x0, 0x0, 0x20, 0x0, 0x81, 0x0, 0x6, 0x6E);
$label = 0x52;
$lenMSB = scalar(@data) >> 8;
$lenLSB = scalar(@data) & 0xFF;
print "RDM test usbpro label $label, data @data, lenLSB $lenLSB, lenMSB $lenMSB\n";
$message = pack("C*", ($som, $label, $lenLSB, $lenMSB));
for (my $i = 0; $i < (($lenMSB << 8) + $lenLSB); $i++) {
    $message .= pack("C*", ($data[$i]));
}
$message .= pack("C*", ($eom));
syswrite PORT,$message;
exit();
sleep(0.1); # 100ms




### UNIT 4 Rainbow ###
my $bri1 = 0x80;
my $bri2 = 0xC0;
$label = 0x06;

for (my $jj = 0; $jj<20; $jj++) {
for (my $i = -9; $i < 300; $i += 3) {
    @data = (0) x 301;
    $data[1+$i-3] = 0x0;
    $data[1+$i-2] = 0x0;
    $data[1+$i-1] = $bri1/2;

    $data[1+$i] = 0x0;
    $data[1+$i+1] = 0x0;
    $data[1+$i+2] = $bri2;
    
    $data[1+$i+3] = 0;
    $data[1+$i+4] = $bri1;
    $data[1+$i+5] = $bri1;
    
    $data[1+$i+6] = 0;
    $data[1+$i+7] = $bri1;
    $data[1+$i+8] = 0;

    $data[1+$i+9] = $bri1;
    $data[1+$i+10] = $bri1;
    $data[1+$i+11] = 0;

    $data[1+$i+12] = $bri1;
    $data[1+$i+13] = 0;
    $data[1+$i+14] = 0;

    $data[1+$i+15] = $bri1/4;
    $data[1+$i+16] = 0;
    $data[1+$i+17] = 0;

    $lenMSB = scalar(@data) >> 8;
    $lenLSB = scalar(@data) & 0xFF;
    $message = pack("C*", ($som, $label, $lenLSB, $lenMSB));
    for (my $i = 0; $i < (($lenMSB << 8) + $lenLSB); $i++) {
        $message .= pack("C*", ($data[$i]));
    }
    $message .= pack("C*", ($eom));
    syswrite PORT,$message;
    sleep(.05);
}
}


### Unit READ ###
print "reading result: ";
sysread PORT,$res,255;
my  $bufhex = $res;
$bufhex =~ s/(.)/sprintf("0x%x ",ord($1))/eg;
print "READ: $bufhex \n\n";


close PORT;

