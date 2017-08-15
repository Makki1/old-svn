#!/usr/bin/perl -w

use strict;

if (@ARGV < 7) {
    print "ARGS: ",scalar(@ARGV)," ?? \n";
    print "give me the full 7 byte data on cmdline as hex, space-separated, without 0x\n";
    print "like\n 50 05 00 00 7f ff 00 10 gives CRC 0xA9(?) \n\n\n";
#    print "OR just a family and start-value:\n./slavecrc.pl e1 100\n gives 100 slave-id's\n";
    exit();
}

my $lscrc=0;
my $lactbit;

for (my $i=0;$i<7;$i++) {
    for (my $bit=1;$bit<256;$bit=$bit*2) {
        if ((hex($ARGV[$i])&$bit)==$bit) {
            $lactbit=1;
        } else {
            $lactbit=0;
        }
        if (($lscrc&1)!=$lactbit) {
            $lscrc=($lscrc>>1)^0x8c;
        } else {
            $lscrc=($lscrc>>1);
        }
    }
}
print "OUT -> :\nCRC8 dezimal: $lscrc \n";
printf ("CRC8 Hex 0x%02X \n\n", $lscrc);


print "Full hex\n";
for (my $i=0;$i<7;$i++) {
    printf ("0x%02X ", hex($ARGV[$i]));
}
printf("0x%02X\n", $lscrc);

#print "\nC:\nconst uint8_t owid[8] = {";
#for (my $i=0;$i<7;$i++) {
#    printf ("0x%02X, ", hex($ARGV[$i]));
#}
#printf("0x%02X };\n\n", $lscrc);


