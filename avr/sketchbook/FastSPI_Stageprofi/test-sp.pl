#!/usr/bin/perl

use strict;
use Time::HiRes qw ( time alarm sleep usleep gettimeofday ); # uS resolution

#DON'T use Device::SerialPort; its bloatware and does many things we don't want
#use POSIX qw(:termios_h); # Used terminal IO library
my $dev = "/dev/usbserial-6493534343335180D060";
$dev = "/dev/ttyACM1";
# no buffers, no fancy shit, just do it
`stty -F /dev/usbserial-6493534343335180D060 38400`;
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

if ($demo) {
print "Demo on\n";
syswrite PORT,"C508L002";
sleep(5);
syswrite PORT,"C508L000";
print "Demo off\n";
sleep(10);
}
print "2/red on\n";
syswrite PORT,"C003L255";
sleep(2);
print "reading: ";
sysread PORT,$res,255;
print "Result $res\n\n";

#test 1-3 green
print "bulkwrite..\n";
binmode PORT;
my $bytes = pack("C*", (0xff, 0x03, 0x0, 0x9, 0x0, 0xff, 0x0, 0x0, 0xff, 0x0, 0x0, 0xff, 0x0));
syswrite PORT,$bytes;
sleep(0.05); # 50ms
print "reading: ";
sysread PORT,$res,255;
print "Result $res\n\n";

print "bulkwrite 0 to FF\n";
my $numchan = 0xFF;
$bytes = pack("C*", (0xff, 0x00, 0x0, $numchan));
for (my $i=0;$i<$numchan;$i++) {
    $bytes .= pack("C", 0x0);
}
syswrite PORT,$bytes;
sleep(0.05); # 50ms
print "reading: ";
sysread PORT,$res,255;
print "Result $res\n\n";


#0xff 0x03 0x0 0x9 0x0 0xff 0x0 0x0 0xff 0x0 0x0 0xff 0x0
#for (@bytes) {
#    syswrite PORT,$_;
#}

#print "wait a little?..\n";
#sleep 3;

close PORT;

