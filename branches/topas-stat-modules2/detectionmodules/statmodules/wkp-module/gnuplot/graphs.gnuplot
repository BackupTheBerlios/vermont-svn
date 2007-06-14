plot 'cusumparams_0.0.0.0_0|6_pca_component_0.txt' using 7:2 title 'g' with lines, 'cusumparams_0.0.0.0_0|6_pca_component_0.txt' using 7:3 title 'N' with lines, 'cusumparams_0.0.0.0_0|6_pca_component_0.txt' using 7:4 title 'alpha' with lines
pause -1
plot 'wkpparams_0.0.0.0_0|6_pca_component_0.txt' using 9:1 title 'Value' with lines, \
     'wkpparams_0.0.0.0_0|6_pca_component_0.txt' using 9:6 title 'p_pcs' axes x1y2 with lines, \
     'wkpparams_0.0.0.0_0|6_pca_component_0.txt' using 9:4 title 'p_ks' axes x1y2 with lines, \
     'wkpparams_0.0.0.0_0|6_pca_component_0.txt' using 9:2 title 'p_wmw' axes x1y2 with lines
pause -1
