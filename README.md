# cpp-timbreID
An audio feature extraction library for the JUCE framework.  
Porting of some of the objects from the Puredata external  [wbrent/timbreID](https://github.com/wbrent/timbreID) [1].  
A wrapper for [aubio/aubio](https://github.com/aubio/aubio)'s onset detector is included.

A earlier version of this library was used for the paper **On the challenges of embedded real-time audio classification** which was submitted to the 25th International Conference on Digital Audio Effects (DAFx20in22).
The code for the timbre classifier mentioned in the article is in [/master/Demos/Demo-guitarTimbreClassifier](https://github.com/domenicostefani/cpp-timbreID/tree/master/Demos/Demo-guitarTimbreClassifier)  
Check the JUCE projects in ```Demos/``` to understand how to use the library.

At an early stage of development, the library was used for the following scientific papers:
- [**A Comparison of Deep Learning Inference Engines for Embedded Real-time Audio Classification**](https://domenicostefani.com/phd_research.html#2022DAFX-1-Comparison) [2]
- [**On the challenges of embedded real-time music information retrieval**](https://domenicostefani.com/phd_research.html#2022DAFX-2-Challenges) [3]


## Build and usage

At the current stage, library source files (in ```src/```) need to be compiled alongside the user's project. However, most of the code is header-only, requiring to add ```include``` to the include path of the project.  
The library also includes a convenient wrapper for Aubio Onset, which requires to compile the library in ```libs/audio```.
To do so, run the script ```libs/build_dependencies.sh```, which build a tested version of Aubio for both amd64 and [Elk Audio OS](https://www.elk.audio/start) arm64/aarch64.



## References
1. W. Brent, *A  timbre  analysis  and  classification  toolkit  for  pure  data*, in Proc. 2010 Int. Computer Music Conference (ICMC 2010), New York, USA, 2010.  
2. Stefani, D., Peroni, S., & Turchet, L. (2022, September). *A Comparison of Deep Learning Inference Engines for Embedded Real-Time Audio Classification*. Proceedings of the 25-Th Int. Conf. on Digital Audio Effects (DAFx20in22), 3, 256–263. Vienna, Austria.  
3. Stefani, D., & Turchet, L. (2022, September). *On the Challenges of Embedded Real-Time Music Information Retrieval*. Proceedings of the 25-Th Int. Conf. on Digital Audio Effects (DAFx20in22), 3, 177–184. Vienna, Austria.


_Domenico Stefani_  
domenico [at] unitn [dot] it
