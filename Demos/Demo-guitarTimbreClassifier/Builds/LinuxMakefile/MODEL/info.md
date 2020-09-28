# Model to load for inference

This is the TF lite trained model that has to be placed in /udata/tensorflow/ in the ELKOS board.
If not, change path in the code that loads the model.

Performances on paper:

[[ 148    1    1    0    5]
 [   0  134    1    0    6]
 [   1    1  167    0   13]
 [   0    0    1  149    4]
 [  13   14   10   15 8591]]
              precision    recall  f1-score   support

           0      0.914     0.955     0.934       155
           1      0.893     0.950     0.921       141
           2      0.928     0.918     0.923       182
           3      0.909     0.968     0.937       154
           4      0.997     0.994     0.995      8643

    accuracy                          0.991      9275
   macro avg      0.928     0.957     0.942      9275
weighted avg      0.991     0.991     0.991      9275
