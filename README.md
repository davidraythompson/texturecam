
TextureCam: Software for Pixel Classification with Random Forests
========

Authors: David R Thompson, Dmitriy Bekker, Brian Bue, Greydon Foil, 
  Kevin Ortega, Kiri Wagstaff, 
Contact: david.r.thompson@jpl.nasa.gov

Copyright 2014, by the California Institute of Technology. ALL RIGHTS RESERVED.
United States Government Sponsorship acknowledged. Any commercial use must be
negotiated with the Office of Technology Transfer at the California Institute
of Technology. 

About TextureCam
--------
This software was developed at the Jet Propulsion Laboratory for automatic
classification and mapping of geologic features and surfaces.   Examples
include distinctions in material composition (rock vs. sediment) and surface
condition (dust deposition and weathering).  We formalize the problem as pixel
classification.  Given a suitably representative training set, the TextureCam
software can learn a model that allows it to predict pixel classes for future
images. 

We use a random forest classification engine similar to Shotton et al [1], and
train this model using labeled examples supplied in advance by the analyst.
The training process optimizes a "decision tree," a sequence of simple
numerical tests applied to local neighborhoods of the image that together
determine the texture classification of each pixel. This procedure is detailed
in [2,3]. After training, the model can extrapolate the statistical
relationships to classify other scenes.  Specifically, it predicts P(C|X) where
C is the pixel class and X represents features of the local image neighborhood.

File Types
--------
The TextureCam software uses a superset of the PGM/PPM image format standard.
It supports one- and three-channel images through standard PGM and PPM headers.
Images must be 8-bit, for now.  It can also read many-channel images using a
custom extension of the standard PPM format; these are created by concatenating
together regular PPMs and PGMs using the utility "catpgm", described below.

In addition to images, the TextureCam code relies on a special ASCII format to
store random forests.  We commonly name these files with the suffix ".rf", but
you can use whichever convention you choose.  Note that the preprocessing and
number of channels used in a training set must match the images to be
classified.

In addition to the raw image data and random forest files, the training process
relies on having manual labels for some of the pixels in the training set.
These labels are provided to the system in the form of a handmade three-channel
PPM image with each class assigned a unique color.  It is not necessary to
classify all pixels in each image.  By convention, black (#000000) pixels are
considered unclassified and are ignored during training.  

Installation and Contents
--------
The TextureCam codebase is self-contained.  In order to build the key  command
line executables, type "make" from the command line.  There are five executable
programs, each with its own special syntax.  The three most important
executables are:

* tcprep: (optional) preprocess an image prior to classification.  It seems
that the most useful preprocessing options are to change the colorspace from
RGB to HSV with the "-c" option (three-channel images only), or to apply
bandpass filtering with an option like "-b 1 51" (single-channel images only) 

* tctrain: train a classifier on one or more pairs of images and labels, and
output a random forest file.  We typically recommend training with a larger
dataset than the default size; specify the number of pixels in your dataset
with an option like "-n 500000"

* tcclass: use a previously-trained random forest model to classify pixels in a
new image.  For speed, we recommend using subsampling (e.g. only classify every
n-th pixel).  This can be implemented using an option like "-s 4"

It also comes with two useful utilities:

* catforest: concatenates two random forest files into a larger one.  It's
useful for distributed training on clusters.

* catpgm: concatenates single- or multi-channel pgm/ppm images together along
the channel dimension.  This can be used to make n-channel images to serve as
training or classifier input.  Make sure your channel ordering is consistent
across all images in training and test sets!

Example
--------
This example trains a simple classifier to detect rock and sediment surfaces in
Mars rover images [4].  First make the texturecam binaries using the command:

> make

Next, take a look at the training data in the "example" subdirectory. Our first
action will be to apply some bandpass filtering to reduce wide-area changes in
illumination.  We'll make a preprocessed copy of both training and test images. 

> ./tcprep -b 1 41 example/training.pgm example/training-prep.pgm

> ./tcprep -b 1 41 example/test.pgm example/test-prep.pgm

The two numbers after the "-b" option represent the width of the small and
large averaging windows comprising the bandpass filter.  Thus, "1 41" will
attenuate features of smaller than 1 pixel or larger than 41 pixels.  Since
there's really no image feature smaller than a pixel this combination basically
amounts to a highpass filter.

The next ingredient is the image labels for training the classifier. It's
straightforward to make new label images using photo-editing software; simply
apply a transparent layer to the original image and paint over it with the
colors of your choice.  Set unlabeled pixels to black, and save the result as
an RGB PPM image.  For this example we've crated a label mask with just two
classes - a blue class corresponding to rocks and a red class indicating
sediment pixels.  Some pixels for the training image are stored in
"example/training-label.ppm" We supply each input/label pair in a list of
arguments to the program tctrain, which uses these to build the random forest. 

> ./tctrain -o rocks.rf example/training-prep.pgm example/training-label.ppm

If we were going to train with a larger image set with 2 or more input/label
pairs, we'd simply list them as additional commands in the order: [input1]
[label1] [input2] [label2] [input3] [label3] etc....

After training we apply the resulting model to create a new labeled image.  We
use the subsampling option (-s 4) to make it go really fast. 

> ./tcclass -s 4 rocks.rf example/test-prep.pgm example/test-autolabel.ppm

The new autolabel.ppm file represents the system's attempt to classify the
image based on the learned random forest file.  Colors correspond to the 
class labels in the original training data.  In this example the blue areas 
signify "rock" surfaces, while the red is "sediment." These predictions 
describe the corresponding pixels of the original test image, test.pgm.  
Note that an area at the margin of the frame remains black; the random forest 
does not have a complete context window for those pixels, so it leaves them
unclassified.

Advanced Tricks
--------
There are lots of ways to improve performance.  It is common practice to flip
or rotate training data to expand the the training set, and as a side benefit
promote rotational or directional invariance. One can also apply filters or
various other object detection methods to the base image, and use the output of
these initial stages as the input to the classifier.  One can break classes
into subclasses and train the classifier on these.  

Another common extension involves "probability maps."  The random forest
predicts a posterior probability distribution over all classes, but the
classification output only shows the Maximum A Posteriori (most probable) class
for each pixel.  To record the full probability information, use the "-p
<<probabililty_file.dat>" option for tcclass.  The probability data is recorded
in 32-bit floating point format; it is the same row/column dimensions as the
image, in Band-In-Pixel (BIP) interleave.  For a random forest having n
classes, the probability map has n+1 channels (channel 0 corresponds to the
"undefined" class).  You can use these full probability data to adjust the
classification thresholds later if certain classes are expected to particularly
likely or unlikely.

Consider training with the "--balanced" option.  This will adjust populations
of training pixels to ensure that there are about the same number of datapoints
from each class in the training data.  This often helps in cases where the
training data is imbalanced - e.g. there are many more pixels of some classes
than others.  It ensures that the model has sufficient datapoints from all
classes to form a reasonable  model of each.  If you wish to incorporate prior
knowledge about the predominance of a particular class in the test environment,
you can incorporate this later with the posterior conditional probabilities
using Bayes' rule.

Finally, consider adjusting the training parameters for tcclass.  It is often 
helpful to increase the default training set size (with "-n 1000000" or 
similar).  It can also be helpful to increase the number of node expansions 
(try "-l 200") or the size of the local context window used for classification 
(try "-w 61").

References
--------
[1] Jamie Shotton, Matthew Johnson, and Roberto Cipolla, Semantic Texton
Forests for Image Categorization and Segmentation, in Proc. IEEE CVPR, 2008.

[2] D. R. Thompson, W. Abbey, A. Allwood, D. Bekker, B. Bornstein,  N. A.
Cabrol, R. Castano, T. Estlin, T. Fuchs, K. L. Wagstaff. Smart Cameras  for
Remote Science Survey. Proceedings of the International Symposium on Artificial
Intelligence, Robotics and Automation in Space, Turin Italy, 2012.

[3] K. L. Wagstaff, D. R. Thompson, W. Abbey, A. Allwood, D. L. Bekker, N. A.
Cabrol, T. Fuchs, and K. Ortega. Smart, texture-sensitive instrument
classification for in situ rock and layer analysis. Geophysical Research
Letters, vol. 40, 2013.

[4] These training and test images come from the Mars Science Laboratory,
courtesy  JPL/Caltech. The training and test images are portions of image
NLA_402736799EDR_F0050104NCAM00449M1
