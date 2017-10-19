#!/usr/bin/perl
use strict;

use threads;
use threads::shared;

use IO::Socket::INET;
use Data::Dumper;
use File::Basename;

my $CONTENT_PATH = ( dirname $0 ) . "/../../arduino/projects/WebPlug/html.src";

# auto-flush on socket
$| = 1;
 
# creating a listening socket
my $server = new IO::Socket::INET (
    LocalHost => '0.0.0.0',
    LocalPort => '7777',
    Proto => 'tcp',
    Listen => 25,
    Reuse => 1
);
die "cannot create socket $!\n" unless $server;
print "server waiting for client connection on port 7777\n";
 

my $client;

while ($client = $server->accept())
{
   threads->create( sub {
       $client->autoflush(1);    # Always a good idea 
       close $server;
       
       my $httpReq = parse_http( $client );
       #print Dumper($httpReq);
       my $httpResp = process_http( $httpReq );
       #print Dumper($httpResp);

       my $data = "HTTP/1.1 " . $httpResp->{code} . " " . $httpResp->{text} . "\r\n";
       
       if( exists $httpResp->{fields} )
       {
         for my $key( keys %{$httpResp->{fields}} )
         {
           $data .= "$key: " . $httpResp->{fields}{$key} . "\r\n";
         }
       }
       $data .= "\r\n";
       if( exists $httpResp->{body} )
       {
         $data .= $httpResp->{body};
       }
 
       $client->send($data);
 
       if( $httpResp->{done} )
       {
         # notify client that response has been sent
         shutdown($client, 1);
       }
   } );
   close $client;        # Only meaningful in the client 
}

exit(0);

sub parse_http
{
  my ($client) = @_;
    # read up to 1024 characters from the connected client
    my $data = "";
    
    do{
      my $buf = "";
      $client->recv($buf, 1024);
      $data .= $buf;
    }while( $data !~ /\r\n\r\n/s );
 
    my %resp;
    
    my @lines = split /\r\n/, $data;
    my $head = shift @lines;
    
    if( $head =~ /(GET|POST) / )
    {
      $resp{method} = $1;
      $head =~ s/(GET|POST) //;
      if( $head =~ /^([^ ]+) HTTP\/\d\.\d/ )
      {
        my $args = $1;
        my $u = $args;
        $u =~ s/\?.*$//g;
        $args =~ s/^.*\?//g;
        my %arg = split /[=\&]/, $args;
        $resp{urlArgs} = \%arg;
        $resp{url} = $u;
        
        my %fields;
        while( my $arg = shift @lines )
        {
          if( $arg =~ /^([\w-]+): (.*)$/ )
          {
            $fields{$1} = $2;
          }
        }
        $resp{fields} = \%fields;
      }
      else
      {
        $resp{method} = 'ERROR';
        $resp{error} = 'Invalid HTTP request';
      }
      
      if( $resp{method} eq 'POST' )
      {
        my $remaining = join("\r\n", @lines);
        my $cnt_len = $resp{fields}{'Content-Length'};
     
        while( length($remaining) < $cnt_len )
        {
          my $buf = "";
          $client->recv($buf, 1024);
          $remaining .= $buf;
        }
        
        $resp{postData} = $remaining;
        my %pargs = split /[=\&]/, $remaining;
        $resp{postArgs} = \%pargs;
      }
    }
    else
    {
      $resp{method} = 'ERROR';
      $resp{error} = 'Invalid HTTP request';
    }
    
    return \%resp;
}

sub simple_response
{
  my ($code, $msg) = @_;

  my %resp;
  $resp{code} = $code;
  $resp{text} = $msg;
  $resp{fields} = {};
  $resp{done} = 1;
  
  return \%resp;
}

sub slurp
{
  my ($file) = @_;
  
  open IF, "<", $file or die "Can't read file: $!";
  my @fc = <IF>;
  close(IF);
  my $cnt = join("", @fc);
  return $cnt;
}

sub content_response
{
  my ($content, $url) = @_;
  
  my %resp;
  $resp{code} = 200;
  $resp{text} = "OK";
  $resp{done} = 1;
  $resp{body} = $content;
      
  $resp{fields} = {};
  $resp{fields}{'Content-Length'} = length($content);
      
  $resp{fields}{'Content-Type'} = "text/json";
  $resp{fields}{'Content-Type'} = "text/html; charset=UTF-8" if( $url =~ /\.html$/ );
  $resp{fields}{'Content-Type'} = "text/css" if( $url =~ /\.css$/ );
  $resp{fields}{'Content-Type'} = "text/javascript" if( $url =~ /\.js$/ );
  $resp{fields}{'Content-Type'} = "image/gif" if( $url =~ /\.ico$/ );
  $resp{fields}{'Connection'} = 'close';
      
  return \%resp;
}

sub process_http
{
  my ($httpReq) = @_;
  if( $httpReq->{method} eq 'ERROR' )
  {
    return simple_response(400, $httpReq->{error});
  }

  my $pthTest = dirname $0;
  my $pthWeb = $CONTENT_PATH;

  my $url = $httpReq->{url};
  $url =~ s/^\///;

  my $webMethod = "$pthTest/" . lc($httpReq->{method}) . '.' . $url . '.pl';
  if( -f $webMethod ) {
    my $body = slurp $webMethod;
    my $resp = eval "$body";
    if( $@ ) {
      print $@ . "\n";
      return simple_response(500, "Internal server error");
    }
    return $resp;
  }

  if( $httpReq->{method} eq 'GET' )
  {
    if( -f "$pthWeb/$url" )
    {
      my $cnt = slurp( "$pthWeb/$url" );
      
      if( $url =~ /\.html$/ )
      {
        my $prep = slurp( "$pthWeb/head-" );
        $cnt = "$prep$cnt";
      }
      return content_response($cnt, $url); 
    }
    elsif( -f "$pthTest/$url" )
    {
      my $cnt = slurp( "$pthTest/$url" );
      return content_response($cnt, $url); 
    }
    elsif( -f "$pthTest/redirect.$url.url" ) {
      my $location = slurp( "$pthTest/redirect.$url.url" );
      my $resp = simple_response(302, "Redirect");
      $resp->{fields}{'Location'} = $location;
      return $resp;
    }
    else
    {
      return simple_response(404, "File not found");
    }
  }
  
  return simple_response(400, "Invalid HTTP request");
}

