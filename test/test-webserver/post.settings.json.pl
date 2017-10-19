print Dumper($httpReq->{postArgs});

my $resp = simple_response(302, "Redirect");
$resp->{fields}{'Location'} = "/beallitasok.html";
return $resp;
