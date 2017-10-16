#!/usr/bin/perl

use strict;
use Data::Dumper;

my $file = slurp('attiny_halo.hex');

my @rows = split /\n/, $file;

my $output = "const uint8_t attiny_code [] = {\n";

for my $row (@rows)
{
  $row =~ s/^\*|\s*$//g;
  
  if( $row =~ /^:([0-9A-Fa-f]{2})([0-9A-Fa-f]{4})([0-9A-Fa-f]{2})([0-9A-Fa-f]*)/ )
  {
    my $num = hex( "0x" . $1 );
    my $addr = hex( "0x" . $2 );
    my $type = hex( "0x" . $3 );
    my $data = $4;

    my $hexdigits = substr($row, 1);
    my @parts = ( $hexdigits =~ m/../g );
    $_ = "0x" . $_ for(@parts);
    
    my $sum = 0;
    $sum += hex($_) for(@parts);
    die "Invalid row: $row" if( $sum & 0xFF ) != 0;
    
    if( $type == 0 )
    {
      # data
      my @array = ( $data =~ m/../g );
      $_ = "0x" . $_ for(@array);
      my $chkSum = pop @array;
      
      $output .= "  " . join(", ", @array) . ",\n";
    }
    elsif ( $type == 1 )
    {
      # terminator
      last;
    }
    else
    {
      die "Unsupported element in hex file!";
    }
  }
}

$output .= "};\n\n";

open OUT, ">", "attiny_halo.h" or die "Can't open attiny_halo.h for write!";
print OUT $output;
close OUT;

exit 0;

sub slurp
{
  my ($name) = @_;
  
  open my $handle, '<:encoding(UTF-8)', $name or die "Can't open '$name' for reading: $!";
  my @file = <$handle>;
  my $cnt = join("", @file);
  close($handle);
  return $cnt;
}
