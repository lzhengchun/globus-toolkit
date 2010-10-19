
use Getopt::Long;
use English;

my $gpath = $ENV{GPT_LOCATION};
if (!defined($gpath))
{
  $gpath = $ENV{GLOBUS_LOCATION};

}
if (!defined($gpath))
{
   die "GPT_LOCATION or GLOBUS_LOCATION needs to be set before running this script"
}

@INC = (@INC, "$gpath/lib/perl");


$setup_gsi_options = join(" ", @ARGV);

if( ! &GetOptions("nonroot|d:s","help!", "default!") ) 
{
   pod2usage(1);
}

if(defined($opt_help))
{
   pod2usage(0);
}

my $globusdir = $ENV{GLOBUS_LOCATION};
my $setupdir = "$globusdir/setup/globus_gcs_b38b4d8c_setup";

my $target_dir = "/etc/grid-security";
my $trusted_certs_dir = $target_dir . "/certificates";

my $myname = "setup-ssl-utils";

print "$myname: Configuring ssl-utils package\n";

#
# Run setup-ssl-utils-sh-scripts. This will:
#   -Create grid-security-config from grid-security-config.in
#   -Create grid-cert-request-config from grid-cert-request-config.in
#

print "Running setup-ssl-utils-sh-scripts...\n";

my $result = `$setupdir/setup-ssl-utils-sh-scripts`;

$result = system("chmod 755 $setupdir/grid-security-config");

if ($result != 0) {
  die "Failed to set permissions on $setupdir/grid-security-config";
}

$result = system("chmod 755 $setupdir/grid-cert-request-config");

if ($result != 0) {
  die "Failed to set permissions on $setupdir/grid-cert-request-config";
}

if(defined($opt_nonroot))
{

    print "

Running: $setupdir/setup-gsi $setup_gsi_options

";

    system("$setupdir/setup-gsi $setup_gsi_options");

    print "
done with setup-ssl-utils.
";

} 
else 
{

   print "
***************************************************************************

Note: To complete setup of the GSI software you need to run the
following script as root to configure your security configuration
directory:

$setupdir/setup-gsi

For further information on using the setup-gsi script, use the -help
option.  The -default option sets this security configuration to be 
the default, and -nonroot can be used on systems where root access is 
not available.

***************************************************************************

$myname: Complete

";

}

sub pod2usage 
{
  my $ex = shift;

  print "setup-ssl-utils [
              -help
              -nonroot[=path] 
                 sets the directory that the security configuration
	         files will be placed in.  If no argument is given,
	         the config files will be placed in \$GLOBUS_LOCATION/etc/
                 and the CA files will be placed in  
                 \$GLOBUS_LOCATION/share/certificates.
              -default
                 will set the CA being installed to the default CA.
                ]\n";

  exit $ex;
}


# End