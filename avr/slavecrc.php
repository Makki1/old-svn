\\$ra enthaelt mind. die ersten 7 Bytes der ID
     $lscrc=0x0;
     for ($i=0;$i<7;$i++) { 
         for ($bit=1;$bit<256;$bit=$bit*2) {
             if (($ra[$i]&$bit)==$bit) $lactbit=1; else $lactbit=0;
             if (($lscrc&1)!=$lactbit) $lscrc=($lscrc>>1)^0x8c; else $lscrc=($lscrc>>1);
          }
     }
     $ra[7]=$lscrc;

