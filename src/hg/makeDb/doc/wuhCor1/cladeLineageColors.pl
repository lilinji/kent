#!/usr/bin/env perl

# Read in a file with 3 columns: sample name, Pangolin lineage, GISAID clade
# (unfortunately GISAID metadata does not include Nextstrain clade so this is only approximate)
# Choose a color for the sample according to the scheme in Figure 1 of
# https://www.eurosurveillance.org/content/10.2807/1560-7917.ES.2020.25.32.2001410
# Figure 1 was generated by Emma Hodcroft of Nextstrain and Aine O'Toole, Pangolin author.

use warnings;
use strict;

my %lin2col = ( # Some, but not all, of the "gray area" where GH and Nextstrain 20A overlap
               "B.1.22" => "#888888",
               "B.1.37" => "#888888",
               "B.1.13" => "#888888",
               "B.1.36" => "#888888",
               "B.1.9"  => "#888888",

               # Some, but not all, of the green B.1 sublineages (G and 20A)
               "B.1.6"  => "#44cc44",
               "B.1.67" => "#88cc88",
               "B.1.5"  => "#44cc44",
               "B.1.72" => "#88cc88",
               "B.1.70" => "#44cc44",
               "B.1.8"  => "#88cc88",
               "B.1.34" => "#44cc44",
               "B.1.71" => "#88cc88",
               "B.1.69" => "#44cc44",
               "B.1.19" => "#88cc88",
               "B.1.23" => "#44cc44",
               "B.1.32" => "#88cc88",
               "B.1.33" => "#44cc44",
               "B.1.35" => "#88cc88",

               # Some, but not all, of the blue B.1 sublineages (GH and 20C)
               "B.1.12" => "#8888ff",
               "B.1.30" => "#8888ff",
               "B.1.38" => "#8888ff",
               "B.1.66" => "#8888ff",
               "B.1.44" => "#8888ff",
               "B.1.40" => "#8888ff",
               "B.1.31" => "#8888ff",
               "B.1.39" => "#8888ff",
               "B.1.41" => "#8888ff",
               "B.1.3"  => "#8888ff",
               "B.1.26" => "#8888ff",
               "B.1.43" => "#8888ff",
               "B.1.29" => "#8888ff"
              );

my %gc2col = ( # GISAID clade coloring
              "S"  => "#ec676d",
              "L"  => "#f79e43",
              "O"  => "#f9d136",
              "V"  => "#faea95",
              "G"  => "#b6d77a",
              "GH" => "#8fd4ed",
              "GR" => "#a692c3"
             );

while (<>) {
  chomp;
  my ($sample, $lineage, $gClade) = split;
  my $color = "#000000";
  if (defined $lin2col{$lineage}) {
    $color = $lin2col{$lineage};
  } elsif ($lineage eq "") {
    # No lineage assigned; fall back on clade.
    if (defined $gc2col{$gClade}) {
      $color = $gc2col{$gClade};
    }
  } else {
    my @linPath = split(/\./, $lineage);
    my $linDepth = scalar @linPath;
    if (! $linPath[0]) {
      die "no linpath[0] for '$lineage'\n";
    } elsif ($linPath[0] eq "C") {
      $linDepth += 4;
    }
    if ($lineage eq "B.1.1" ||
        substr($lineage, 0, 6) eq "B.1.1." ||
        substr($lineage, 0, 1) eq "C") {
      # Shades of purple -- minimum depth is 3, use even/odd to alternate colors
      my $endNum = ($lineage =~ m/\.(\d+)$/) ? ($1 + 0) : 0;
      if ($linDepth > 4) {
        $color = ($endNum & 1) ? "#662288" : "#8822aa";
      } elsif ($linDepth > 3) {
        $color = ($endNum & 1) ? "#aa44cc" : "#cc44ee";
      } else {
        $color = "#cc88ff";
      }
    } elsif ($lineage eq "B.1" ||
             substr($lineage, 0, 4) eq "B.1.") {
      # Green, blue or gray... complicated.  We really need a tree-based coloring method.
      my $endNum = ($lineage =~ m/\.(\d+)$/) ? ($1 + 0) : 0;
      if (substr($lineage, 0, 6) eq "B.1.5.") {
        $color = ($endNum & 1) ? "#88dd88" : "#88ee88";
      } else {
        # Unfortunately now GISAID clade is all we have to go on; don't know where 20A overlaps GH.
        if ($gClade eq "G") {
          # green
          $color = ($endNum & 1) ? "#66cc66" : "#88cc88";
        } elsif ($gClade eq "GH") {
          # blue
          $color = ($endNum & 1) ? "#6666ff" : "#8888ff";
        } else {
          # gray
          $color = ($endNum & 1) ? "#aaaaaa" : "#cccccc";
        }
      }
    } elsif (substr($lineage, 0, 1) eq "B") {
      # Shades of orange (not as much depth in B.>1)
      my $endNum = ($lineage =~ m/\.(\d+)$/) ? ($1 + 0) : 0;
      if ($linDepth >= 3) {
        $color = ($endNum & 1) ? "#ee6640" : "#ff774a";
      } elsif ($linDepth == 2) {
        $color = ($endNum & 1) ? "#f07850" : "#ff8844";
      } else {
        $color = "#ff9060";
      }
    } elsif (substr($lineage, 0, 1) eq "A") {
      # Shades of red
      my $endNum = ($lineage =~ m/\.(\d+)$/) ? ($1 + 0) : 0;
      if ($linDepth >= 3) {
        $color = ($endNum & 1) ? "#bb2222" : "#cc2222";
      } elsif ($linDepth == 2) {
        $color = ($endNum & 1) ? "#dd2222" : "#ee3333";
      } else {
        $color = "#ff6666";
      }
    }
  }
  print "$sample\t$color\n";
}
