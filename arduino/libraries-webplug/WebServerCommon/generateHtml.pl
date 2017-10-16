#!/usr/bin/perl

use strict;
use File::Copy qw(copy);

# delete the generated directory
opendir(my $dhr, "html") || die "Can't open html: $!";
while (my $file = readdir $dhr) {
  next if ($file eq '.' ) || ($file eq '..' );
  unlink "html/$file";
}
closedir $dhr;

# read head- file
my $head = slurp("html.src/head-");

# copy files
opendir(my $dh, "html.src") || die "Can't open html.src: $!";
while (readdir $dh) {
  if( $_ =~ /\.(js|css|ico)$/ ) {
    copy("html.src/$_", "html/$_");
  }
  if( $_ =~ /\.html$/ ) {
    my $html = $head . slurp("html.src/$_");
    open(my $wh, ">", "html/$_") || die "Can't open html/$_: $!";
    print $wh $html;
    close $wh;
  }
}
closedir $dh;

system("gzip html/*.html html/*.js html/*.css");

sub slurp
{
  my ($file) = @_;

  local $/ = undef;
  open(my $fh, "<", $file) || die "Can't open $file: $!";
  my $res = <$fh>;
  close $fh;
  return $res;
}
