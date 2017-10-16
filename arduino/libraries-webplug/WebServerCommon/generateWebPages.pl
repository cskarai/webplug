#!/usr/bin/perl

use strict;
use Data::Dumper;

my @names;
my %content;
my @urlName;
my @contentName;
my %zipped;

opendir(my $dh, "html/") || die "Can't open html/: $!";
while (readdir $dh) {
  next if ! -f "html/$_";
  push @names, $_;
  my $cnt = slurp( "html/$_" );
  $content{$_} = $cnt;
}
closedir $dh;

@names = sort @names;

my $output = "// this is a generated file, please don't edit!\n\n" .
             "#include \<WebServerCommon.h\>\n\n";

for( my $i=0; $i < @names; $i++)
{
  my $origName = $names[$i];
  my $urlName = $origName;
  my $name = $origName;
  $name =~ s/[\.\-]/_/g;
  
  if( $urlName =~ /\.gz/ ) {
    $zipped{$origName} = 1;
    $urlName =~ s/\.gz//;
  } else {
    $zipped{$origName} = 0;
  }

  push @urlName, "${name}_url";
  $output .=  "const char ${name}_url [] PROGMEM = \"/$urlName\";\n\n";

  push @contentName, "$name";
  $output .=  "const uint8_t $name [] PROGMEM = {";

  my $cnt = $content{$origName};
  
  for( my $ik=0; $ik < length($cnt); $ik++ ) {
    my $chr = ord( substr($cnt, $ik, 1));
    if(($ik & 0x1F) == 0) {
      $output .=  "\n  ";
    }
    $output .= sprintf("0x%02X, ", $chr);
  }
  $output .=  "\n";
  
  my $part = substr($cnt, 0, 16);
  $output .= "};\n\n";
}

$output .= "struct WEBCONTENT {\n  const char * url;\n  const uint8_t * content;\n  int size;\n  bool zipped;\n};\n\n";

$output .= "struct WEBCONTENT webPages[] = {\n";
for( my $i=0; $i < @names; $i++)
{
  $output .= "{.url=" . $urlName[$i] . ",.content=" . $contentName[$i] . ",.size=" . length($content{$names[$i]}) . ",.zipped=" . ($zipped{$names[$i]} ? 'true' : 'false') ."},\n";
}
$output .= "};\n\n";

open OUT, ">WebPages.h" or die "Can't open WebPages.h for write!";
print OUT $output;
close OUT;


sub slurp
{
  my ($name) = @_;
 
  local $/ = undef;

  open my $handle, $name or die "Can't open '$name' for reading: $!";
  binmode $handle;
  my $cnt = <$handle>;
  close($handle);
  return $cnt;
}
