#!/opt/local/bin/perl
# Set up the serial port
use Device::SerialPort;
my $port = Device::SerialPort->new("/dev/tty.usbserial-A900ccgj");

sub readFile { 
  my($name) = @_;
  my($buffer);
  my(@outbuf) = ();
  open(INFILE,"<",$name) or die "Cannot open $name\n";
  binmode(INFILE);
  while(read(INFILE,$buffer,256000) != 0) {
    push(@outbuf,split(//,$buffer));
  }
  close(INFILE);
  return @outbuf;
}

$| = 1;
$port->baudrate(19200); 
$port->databits(8); 
$port->parity("none");
$port->stopbits(1);

# wait for 3 X's
$port->are_match("XXX");
while (1) {
  my $c = $port->lookfor();
  last if ($c ne "");
}

while (@ARGV) {
  my $nextfile = shift(@ARGV);
  my @buf = readFile($nextfile);
  my $len = @buf;
  print "Sending $nextfile, $len bytes ...\n";
  my $counter = 0;
  while (@buf) {
    my $byte = ord(shift(@buf));
    my $hexstr = sprintf("%02X\n",$byte);
    $port->write("$hexstr");
    $counter++;
    if ($counter%256 == 0) {
      $port->are_match("PAGE");
      while (1) {
	my $p = $port->lookfor();
	last if ($p ne "");
      }
      print ".";
    }
    # print "$hexstr";
  }
  $port->write("XX\n");
  print "... $nextfile sent\n";
  my $timo = time()+3;
  $port->are_match("ZZZ");
  while (time() < $timo) {
    my $c = $port->lookfor();
    next if $c eq "";
    print "ACK received\n";
    last;
  }
  if (time() >= $timo) { die("no ACK for $nextfile") };
}
$port->write(" ZZ\n");
print "wait to write table\n";
sleep 3;
print "DONE\n";

