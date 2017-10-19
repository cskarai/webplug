my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime();

my $time = sprintf("%02d:%02d:%02d", $hour, $min, $sec);
my $date = sprintf("%04d-%02d-%02d", $year + 1900, $mon, $mday);

my @daynames = ('Vasárnap', 'Hétfő', 'Kedd', 'Szerda', 'Csütörtök', 'Péntek', 'Szombat',);
my $dayName = $daynames[$wday];

my $content = '{"list":[["sntp_time","' . $time . '","innerHTML"],["sntp_date","' . $date . '","innerHTML"],["sntp_day","' . $dayName . '","innerHTML"]]}';
return content_response($content, $url)
