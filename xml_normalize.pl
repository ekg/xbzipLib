####################################################################
#
# Takes an XML file and turns it into a normal form, such
# that attributes are sorted alphabetically, one space
# before/after =, and double quotes for attribute values.
# 
# Paolo Ferragina, Dipartimento di Informatica, 
# Universita' di Pisa, Italy
# email: ferragina@di.unipi.it
#
# Novembre 2004
#
####################################################################



##############################################################
# Definition of the XML parser
##############################################################
use XML::Parser;

my $parser = new XML::Parser( Handlers => {
    Start =>   \&handle_tag_start,
    End =>   \&handle_tag_end,
    Default => \&handle_default,
}, NoExpand => 1 );

my $total_size = 0; # Allows to count the tree size
my $num_leaves = 0;
my $max_depth = 0;
my $avg_depth = 0;
my $current_depth = 0;
my %hash = (); # Allows to count the distinct tag/attrs
my $dict_size = 0; # Allows to count the dictionary size
my $file_size = 0;

##############################################################
# Global variables
##############################################################

$xmlfile = shift @ARGV; # XML file to process

if ($xmlfile =~ m/[^.]+$/) {
    $rootname = $`;
    $extname = $&;
    } else {
    die "No extension is present in $xmlfile\n\n";
    }
    
if ($extname eq "xml"){
    $xmlfile_out = $rootname . "normal.xml"; 
} else {
    die "Not correct XML extension in $xmlfile\n\n";
    }
        
#generate the normalized file
open(OUT,">$xmlfile_out")
    || die "Error in opening $xmlfile_out!";

# Parse the XML file $xmlfile and store in %h the XML-paths (reversed)
$parser->parsefile( $xmlfile );
close($xmlfile_out);

print "\n\n ====== Statistics =====\n";
$file_size = (stat($xmlfile))[7];
print " File size = $file_size,\n";
print " Tree size =  $total_size,\n";
print " Dictionary size (tags+attrs)= " . keys( %hash ) . "\n";
print " Max tree depth = " . $max_depth . "\n";
$avg_depth = $avg_depth / $num_leaves;
print " Max tree depth = " . $avg_depth . "\n\n";

exit;

  
####################################################################
#
# XML Handlers definitions
#
####################################################################


##################################### 
# Event: tag opening
##################################### 

sub handle_tag_start {
    my( $expat, $element, %attrs ) = @_;

    print OUT "<$element";
    
    # For statistics
    $total_size += 1;
    $hash{"\<$element"} = 1;

        
    # Prints attributes sorted and " and one space
    foreach $attribute_name (sort (keys %attrs)) {

        # attribute names
        $tmp_str = $attrs{$attribute_name};
        $tmp_str =~ s/&/ /;
        print OUT " $attribute_name=\"$tmp_str\"";
        

    # For statistics
    $total_size += 1;
    $hash{"\@$attribute_name"} = 1;

    }
    
    $current_depth++;
    if ($current_depth > $max_depth) { $max_depth = $current_depth; }
    print OUT ">";
}

##################################### 
# Event: Manage Tag End 
#        (appended also in case of < .../>
##################################### 

sub handle_tag_end {
    my( $expat, $element ) = @_;
    print OUT "</$element>";

    # For statistics
    $total_size += 1;
    $current_depth--;
    }


##################################### 
# Event: Manage everything else
# Here go the entities, the PI and the comments....
##################################### 

sub handle_default {
my( $expat, $string ) = @_;
    $total_size += 1;
    $string =~ s/\r$//;

    if ($string =~ m/&/) { return() ;}

    print OUT $string;
    $avg_depth += ($current_depth+1); 
    $num_leaves++;
}
