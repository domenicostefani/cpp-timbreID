/*
  ==============================================================================

  Plugin Processor

  DEMO PROJECT - Feature Extractor Plugin

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 10th September 2020

  ==============================================================================
*/


#pragma once

#include <JuceHeader.h>
#include "juce_timbreID.h"
#include "postOnsetTimer.h"
#include "csv_saver.h"

#define LOG_LATENCY
#define DO_LOG_TO_FILE
/**
*/
class DemoProcessor : public AudioProcessor,
                      public tid::aubio::Onset<float>::Listener
{
public:

public:
    //===================== ONSET DETECTION PARAMS =============================

    // More about the parameter choice at
    // https://github.com/domenicostefani/aubioonset-performanceanalysis/blob/main/report/complessive/Aubio_Study.pdf
    //
    const unsigned int AUBIO_WINDOW_SIZE = 256;
    const unsigned int AUBIO_HOP_SIZE = 64;
    const float ONSET_THRESHOLD = 1.21f;
    const float ONSET_MINIOI = 0.02f;  //20 ms debounce
    const float SILENCE_THRESHOLD = -53.0f;
    const tid::aubio::OnsetMethod ONSET_METHOD = tid::aubio::OnsetMethod::mkl;
    const bool DISABLE_ADAPTIVE_WHITENING = true; // remember to implement in initialization module

    //==================== FEATURE EXTRACTION PARAMS ===========================
    const unsigned int FEATUREEXT_WINDOW_SIZE = 704; // 11 Blocks of 64samples, 14.66ms
    const float BARK_SPACING = 0.5;
    const float BARK_BOUNDARY = 8.5;
    const float MEL_SPACING = 100;

    //========================= ONSET DETECTION ================================
    tid::aubio::Onset<float> aubioOnset{AUBIO_WINDOW_SIZE,
                                        AUBIO_HOP_SIZE,
                                        ONSET_THRESHOLD,
                                        ONSET_MINIOI,
                                        SILENCE_THRESHOLD,
                                        ONSET_METHOD};
    void onsetDetected (tid::aubio::Onset<float> *);

    PostOnsetTimer postOnsetTimer;
    void onsetDetectedRoutine();

    //======================== FEATURE EXTRACTION ==============================
    tid::Bfcc<float> bfcc{FEATUREEXT_WINDOW_SIZE, BARK_SPACING};
    tid::Cepstrum<float> cepstrum{FEATUREEXT_WINDOW_SIZE};
    tid::AttackTime<float> attackTime{FEATUREEXT_WINDOW_SIZE};
    tid::BarkSpecBrightness<float> barkSpecBrightness{FEATUREEXT_WINDOW_SIZE,
                                                      BARK_SPACING,
                                                      BARK_BOUNDARY};
    tid::BarkSpec<float> barkSpec{FEATUREEXT_WINDOW_SIZE, BARK_SPACING};
    tid::Mfcc<float> mfcc{FEATUREEXT_WINDOW_SIZE, MEL_SPACING};
    tid::PeakSample<float> peakSample{FEATUREEXT_WINDOW_SIZE};
    tid::ZeroCrossing<float> zeroCrossing{FEATUREEXT_WINDOW_SIZE};

    /**    Feature Vector    **/
    static const unsigned int VECTOR_SIZE = 498; // Number of features (preallocation) (658 with WindowSize = 1024)
    static const unsigned int CSV_FLOAT_PRECISION = 8;
    const int BARKSPEC_SIZE = 50;
    std::vector<float> barkSpecRes;
    const int BFCC_RES_SIZE = 50;
    std::vector<float> bfccRes;
    const int CEPSTRUM_RES_SIZE = 353; // WindowSize/2+1 (It would be 513 with 1025)
    std::vector<float> cepstrumRes;
    const int MFCC_RES_SIZE = 38;
    std::vector<float> mfccRes;
    void computeFeatureVector(float featureVector[]);          // Compose vector

    // const std::vector<std::string> header = {"attackTime_peaksamp","attackTime_attackStartIdx","attackTime_value","barkSpecBrightness","barkSpec_1","barkSpec_2","barkSpec_3","barkSpec_4","barkSpec_5","barkSpec_6","barkSpec_7","barkSpec_8","barkSpec_9","barkSpec_10","barkSpec_11","barkSpec_12","barkSpec_13","barkSpec_14","barkSpec_15","barkSpec_16","barkSpec_17","barkSpec_18","barkSpec_19","barkSpec_20","barkSpec_21","barkSpec_22","barkSpec_23","barkSpec_24","barkSpec_25","barkSpec_26","barkSpec_27","barkSpec_28","barkSpec_29","barkSpec_30","barkSpec_31","barkSpec_32","barkSpec_33","barkSpec_34","barkSpec_35","barkSpec_36","barkSpec_37","barkSpec_38","barkSpec_39","barkSpec_40","barkSpec_41","barkSpec_42","barkSpec_43","barkSpec_44","barkSpec_45","barkSpec_46","barkSpec_47","barkSpec_48","barkSpec_49","barkSpec_50","bfcc_1","bfcc_2","bfcc_3","bfcc_4","bfcc_5","bfcc_6","bfcc_7","bfcc_8","bfcc_9","bfcc_10","bfcc_11","bfcc_12","bfcc_13","bfcc_14","bfcc_15","bfcc_16","bfcc_17","bfcc_18","bfcc_19","bfcc_20","bfcc_21","bfcc_22","bfcc_23","bfcc_24","bfcc_25","bfcc_26","bfcc_27","bfcc_28","bfcc_29","bfcc_30","bfcc_31","bfcc_32","bfcc_33","bfcc_34","bfcc_35","bfcc_36","bfcc_37","bfcc_38","bfcc_39","bfcc_40","bfcc_41","bfcc_42","bfcc_43","bfcc_44","bfcc_45","bfcc_46","bfcc_47","bfcc_48","bfcc_49","bfcc_50","cepstrum_1","cepstrum_2","cepstrum_3","cepstrum_4","cepstrum_5","cepstrum_6","cepstrum_7","cepstrum_8","cepstrum_9","cepstrum_10","cepstrum_11","cepstrum_12","cepstrum_13","cepstrum_14","cepstrum_15","cepstrum_16","cepstrum_17","cepstrum_18","cepstrum_19","cepstrum_20","cepstrum_21","cepstrum_22","cepstrum_23","cepstrum_24","cepstrum_25","cepstrum_26","cepstrum_27","cepstrum_28","cepstrum_29","cepstrum_30","cepstrum_31","cepstrum_32","cepstrum_33","cepstrum_34","cepstrum_35","cepstrum_36","cepstrum_37","cepstrum_38","cepstrum_39","cepstrum_40","cepstrum_41","cepstrum_42","cepstrum_43","cepstrum_44","cepstrum_45","cepstrum_46","cepstrum_47","cepstrum_48","cepstrum_49","cepstrum_50","cepstrum_51","cepstrum_52","cepstrum_53","cepstrum_54","cepstrum_55","cepstrum_56","cepstrum_57","cepstrum_58","cepstrum_59","cepstrum_60","cepstrum_61","cepstrum_62","cepstrum_63","cepstrum_64","cepstrum_65","cepstrum_66","cepstrum_67","cepstrum_68","cepstrum_69","cepstrum_70","cepstrum_71","cepstrum_72","cepstrum_73","cepstrum_74","cepstrum_75","cepstrum_76","cepstrum_77","cepstrum_78","cepstrum_79","cepstrum_80","cepstrum_81","cepstrum_82","cepstrum_83","cepstrum_84","cepstrum_85","cepstrum_86","cepstrum_87","cepstrum_88","cepstrum_89","cepstrum_90","cepstrum_91","cepstrum_92","cepstrum_93","cepstrum_94","cepstrum_95","cepstrum_96","cepstrum_97","cepstrum_98","cepstrum_99","cepstrum_100","cepstrum_101","cepstrum_102","cepstrum_103","cepstrum_104","cepstrum_105","cepstrum_106","cepstrum_107","cepstrum_108","cepstrum_109","cepstrum_110","cepstrum_111","cepstrum_112","cepstrum_113","cepstrum_114","cepstrum_115","cepstrum_116","cepstrum_117","cepstrum_118","cepstrum_119","cepstrum_120","cepstrum_121","cepstrum_122","cepstrum_123","cepstrum_124","cepstrum_125","cepstrum_126","cepstrum_127","cepstrum_128","cepstrum_129","cepstrum_130","cepstrum_131","cepstrum_132","cepstrum_133","cepstrum_134","cepstrum_135","cepstrum_136","cepstrum_137","cepstrum_138","cepstrum_139","cepstrum_140","cepstrum_141","cepstrum_142","cepstrum_143","cepstrum_144","cepstrum_145","cepstrum_146","cepstrum_147","cepstrum_148","cepstrum_149","cepstrum_150","cepstrum_151","cepstrum_152","cepstrum_153","cepstrum_154","cepstrum_155","cepstrum_156","cepstrum_157","cepstrum_158","cepstrum_159","cepstrum_160","cepstrum_161","cepstrum_162","cepstrum_163","cepstrum_164","cepstrum_165","cepstrum_166","cepstrum_167","cepstrum_168","cepstrum_169","cepstrum_170","cepstrum_171","cepstrum_172","cepstrum_173","cepstrum_174","cepstrum_175","cepstrum_176","cepstrum_177","cepstrum_178","cepstrum_179","cepstrum_180","cepstrum_181","cepstrum_182","cepstrum_183","cepstrum_184","cepstrum_185","cepstrum_186","cepstrum_187","cepstrum_188","cepstrum_189","cepstrum_190","cepstrum_191","cepstrum_192","cepstrum_193","cepstrum_194","cepstrum_195","cepstrum_196","cepstrum_197","cepstrum_198","cepstrum_199","cepstrum_200","cepstrum_201","cepstrum_202","cepstrum_203","cepstrum_204","cepstrum_205","cepstrum_206","cepstrum_207","cepstrum_208","cepstrum_209","cepstrum_210","cepstrum_211","cepstrum_212","cepstrum_213","cepstrum_214","cepstrum_215","cepstrum_216","cepstrum_217","cepstrum_218","cepstrum_219","cepstrum_220","cepstrum_221","cepstrum_222","cepstrum_223","cepstrum_224","cepstrum_225","cepstrum_226","cepstrum_227","cepstrum_228","cepstrum_229","cepstrum_230","cepstrum_231","cepstrum_232","cepstrum_233","cepstrum_234","cepstrum_235","cepstrum_236","cepstrum_237","cepstrum_238","cepstrum_239","cepstrum_240","cepstrum_241","cepstrum_242","cepstrum_243","cepstrum_244","cepstrum_245","cepstrum_246","cepstrum_247","cepstrum_248","cepstrum_249","cepstrum_250","cepstrum_251","cepstrum_252","cepstrum_253","cepstrum_254","cepstrum_255","cepstrum_256","cepstrum_257","cepstrum_258","cepstrum_259","cepstrum_260","cepstrum_261","cepstrum_262","cepstrum_263","cepstrum_264","cepstrum_265","cepstrum_266","cepstrum_267","cepstrum_268","cepstrum_269","cepstrum_270","cepstrum_271","cepstrum_272","cepstrum_273","cepstrum_274","cepstrum_275","cepstrum_276","cepstrum_277","cepstrum_278","cepstrum_279","cepstrum_280","cepstrum_281","cepstrum_282","cepstrum_283","cepstrum_284","cepstrum_285","cepstrum_286","cepstrum_287","cepstrum_288","cepstrum_289","cepstrum_290","cepstrum_291","cepstrum_292","cepstrum_293","cepstrum_294","cepstrum_295","cepstrum_296","cepstrum_297","cepstrum_298","cepstrum_299","cepstrum_300","cepstrum_301","cepstrum_302","cepstrum_303","cepstrum_304","cepstrum_305","cepstrum_306","cepstrum_307","cepstrum_308","cepstrum_309","cepstrum_310","cepstrum_311","cepstrum_312","cepstrum_313","cepstrum_314","cepstrum_315","cepstrum_316","cepstrum_317","cepstrum_318","cepstrum_319","cepstrum_320","cepstrum_321","cepstrum_322","cepstrum_323","cepstrum_324","cepstrum_325","cepstrum_326","cepstrum_327","cepstrum_328","cepstrum_329","cepstrum_330","cepstrum_331","cepstrum_332","cepstrum_333","cepstrum_334","cepstrum_335","cepstrum_336","cepstrum_337","cepstrum_338","cepstrum_339","cepstrum_340","cepstrum_341","cepstrum_342","cepstrum_343","cepstrum_344","cepstrum_345","cepstrum_346","cepstrum_347","cepstrum_348","cepstrum_349","cepstrum_350","cepstrum_351","cepstrum_352","cepstrum_353","cepstrum_354","cepstrum_355","cepstrum_356","cepstrum_357","cepstrum_358","cepstrum_359","cepstrum_360","cepstrum_361","cepstrum_362","cepstrum_363","cepstrum_364","cepstrum_365","cepstrum_366","cepstrum_367","cepstrum_368","cepstrum_369","cepstrum_370","cepstrum_371","cepstrum_372","cepstrum_373","cepstrum_374","cepstrum_375","cepstrum_376","cepstrum_377","cepstrum_378","cepstrum_379","cepstrum_380","cepstrum_381","cepstrum_382","cepstrum_383","cepstrum_384","cepstrum_385","cepstrum_386","cepstrum_387","cepstrum_388","cepstrum_389","cepstrum_390","cepstrum_391","cepstrum_392","cepstrum_393","cepstrum_394","cepstrum_395","cepstrum_396","cepstrum_397","cepstrum_398","cepstrum_399","cepstrum_400","cepstrum_401","cepstrum_402","cepstrum_403","cepstrum_404","cepstrum_405","cepstrum_406","cepstrum_407","cepstrum_408","cepstrum_409","cepstrum_410","cepstrum_411","cepstrum_412","cepstrum_413","cepstrum_414","cepstrum_415","cepstrum_416","cepstrum_417","cepstrum_418","cepstrum_419","cepstrum_420","cepstrum_421","cepstrum_422","cepstrum_423","cepstrum_424","cepstrum_425","cepstrum_426","cepstrum_427","cepstrum_428","cepstrum_429","cepstrum_430","cepstrum_431","cepstrum_432","cepstrum_433","cepstrum_434","cepstrum_435","cepstrum_436","cepstrum_437","cepstrum_438","cepstrum_439","cepstrum_440","cepstrum_441","cepstrum_442","cepstrum_443","cepstrum_444","cepstrum_445","cepstrum_446","cepstrum_447","cepstrum_448","cepstrum_449","cepstrum_450","cepstrum_451","cepstrum_452","cepstrum_453","cepstrum_454","cepstrum_455","cepstrum_456","cepstrum_457","cepstrum_458","cepstrum_459","cepstrum_460","cepstrum_461","cepstrum_462","cepstrum_463","cepstrum_464","cepstrum_465","cepstrum_466","cepstrum_467","cepstrum_468","cepstrum_469","cepstrum_470","cepstrum_471","cepstrum_472","cepstrum_473","cepstrum_474","cepstrum_475","cepstrum_476","cepstrum_477","cepstrum_478","cepstrum_479","cepstrum_480","cepstrum_481","cepstrum_482","cepstrum_483","cepstrum_484","cepstrum_485","cepstrum_486","cepstrum_487","cepstrum_488","cepstrum_489","cepstrum_490","cepstrum_491","cepstrum_492","cepstrum_493","cepstrum_494","cepstrum_495","cepstrum_496","cepstrum_497","cepstrum_498","cepstrum_499","cepstrum_500","cepstrum_501","cepstrum_502","cepstrum_503","cepstrum_504","cepstrum_505","cepstrum_506","cepstrum_507","cepstrum_508","cepstrum_509","cepstrum_510","cepstrum_511","cepstrum_512","cepstrum_513","mfcc_1","mfcc_2","mfcc_3","mfcc_4","mfcc_5","mfcc_6","mfcc_7","mfcc_8","mfcc_9","mfcc_10","mfcc_11","mfcc_12","mfcc_13","mfcc_14","mfcc_15","mfcc_16","mfcc_17","mfcc_18","mfcc_19","mfcc_20","mfcc_21","mfcc_22","mfcc_23","mfcc_24","mfcc_25","mfcc_26","mfcc_27","mfcc_28","mfcc_29","mfcc_30","mfcc_31","mfcc_32","mfcc_33","mfcc_34","mfcc_35","mfcc_36","mfcc_37","mfcc_38","peakSample_value","peakSample_index","zeroCrossing"};
    const std::vector<std::string> header = {"attackTime_peaksamp","attackTime_attackStartIdx","attackTime_value","barkSpecBrightness","barkSpec_1","barkSpec_2","barkSpec_3","barkSpec_4","barkSpec_5","barkSpec_6","barkSpec_7","barkSpec_8","barkSpec_9","barkSpec_10","barkSpec_11","barkSpec_12","barkSpec_13","barkSpec_14","barkSpec_15","barkSpec_16","barkSpec_17","barkSpec_18","barkSpec_19","barkSpec_20","barkSpec_21","barkSpec_22","barkSpec_23","barkSpec_24","barkSpec_25","barkSpec_26","barkSpec_27","barkSpec_28","barkSpec_29","barkSpec_30","barkSpec_31","barkSpec_32","barkSpec_33","barkSpec_34","barkSpec_35","barkSpec_36","barkSpec_37","barkSpec_38","barkSpec_39","barkSpec_40","barkSpec_41","barkSpec_42","barkSpec_43","barkSpec_44","barkSpec_45","barkSpec_46","barkSpec_47","barkSpec_48","barkSpec_49","barkSpec_50","bfcc_1","bfcc_2","bfcc_3","bfcc_4","bfcc_5","bfcc_6","bfcc_7","bfcc_8","bfcc_9","bfcc_10","bfcc_11","bfcc_12","bfcc_13","bfcc_14","bfcc_15","bfcc_16","bfcc_17","bfcc_18","bfcc_19","bfcc_20","bfcc_21","bfcc_22","bfcc_23","bfcc_24","bfcc_25","bfcc_26","bfcc_27","bfcc_28","bfcc_29","bfcc_30","bfcc_31","bfcc_32","bfcc_33","bfcc_34","bfcc_35","bfcc_36","bfcc_37","bfcc_38","bfcc_39","bfcc_40","bfcc_41","bfcc_42","bfcc_43","bfcc_44","bfcc_45","bfcc_46","bfcc_47","bfcc_48","bfcc_49","bfcc_50","cepstrum_1","cepstrum_2","cepstrum_3","cepstrum_4","cepstrum_5","cepstrum_6","cepstrum_7","cepstrum_8","cepstrum_9","cepstrum_10","cepstrum_11","cepstrum_12","cepstrum_13","cepstrum_14","cepstrum_15","cepstrum_16","cepstrum_17","cepstrum_18","cepstrum_19","cepstrum_20","cepstrum_21","cepstrum_22","cepstrum_23","cepstrum_24","cepstrum_25","cepstrum_26","cepstrum_27","cepstrum_28","cepstrum_29","cepstrum_30","cepstrum_31","cepstrum_32","cepstrum_33","cepstrum_34","cepstrum_35","cepstrum_36","cepstrum_37","cepstrum_38","cepstrum_39","cepstrum_40","cepstrum_41","cepstrum_42","cepstrum_43","cepstrum_44","cepstrum_45","cepstrum_46","cepstrum_47","cepstrum_48","cepstrum_49","cepstrum_50","cepstrum_51","cepstrum_52","cepstrum_53","cepstrum_54","cepstrum_55","cepstrum_56","cepstrum_57","cepstrum_58","cepstrum_59","cepstrum_60","cepstrum_61","cepstrum_62","cepstrum_63","cepstrum_64","cepstrum_65","cepstrum_66","cepstrum_67","cepstrum_68","cepstrum_69","cepstrum_70","cepstrum_71","cepstrum_72","cepstrum_73","cepstrum_74","cepstrum_75","cepstrum_76","cepstrum_77","cepstrum_78","cepstrum_79","cepstrum_80","cepstrum_81","cepstrum_82","cepstrum_83","cepstrum_84","cepstrum_85","cepstrum_86","cepstrum_87","cepstrum_88","cepstrum_89","cepstrum_90","cepstrum_91","cepstrum_92","cepstrum_93","cepstrum_94","cepstrum_95","cepstrum_96","cepstrum_97","cepstrum_98","cepstrum_99","cepstrum_100","cepstrum_101","cepstrum_102","cepstrum_103","cepstrum_104","cepstrum_105","cepstrum_106","cepstrum_107","cepstrum_108","cepstrum_109","cepstrum_110","cepstrum_111","cepstrum_112","cepstrum_113","cepstrum_114","cepstrum_115","cepstrum_116","cepstrum_117","cepstrum_118","cepstrum_119","cepstrum_120","cepstrum_121","cepstrum_122","cepstrum_123","cepstrum_124","cepstrum_125","cepstrum_126","cepstrum_127","cepstrum_128","cepstrum_129","cepstrum_130","cepstrum_131","cepstrum_132","cepstrum_133","cepstrum_134","cepstrum_135","cepstrum_136","cepstrum_137","cepstrum_138","cepstrum_139","cepstrum_140","cepstrum_141","cepstrum_142","cepstrum_143","cepstrum_144","cepstrum_145","cepstrum_146","cepstrum_147","cepstrum_148","cepstrum_149","cepstrum_150","cepstrum_151","cepstrum_152","cepstrum_153","cepstrum_154","cepstrum_155","cepstrum_156","cepstrum_157","cepstrum_158","cepstrum_159","cepstrum_160","cepstrum_161","cepstrum_162","cepstrum_163","cepstrum_164","cepstrum_165","cepstrum_166","cepstrum_167","cepstrum_168","cepstrum_169","cepstrum_170","cepstrum_171","cepstrum_172","cepstrum_173","cepstrum_174","cepstrum_175","cepstrum_176","cepstrum_177","cepstrum_178","cepstrum_179","cepstrum_180","cepstrum_181","cepstrum_182","cepstrum_183","cepstrum_184","cepstrum_185","cepstrum_186","cepstrum_187","cepstrum_188","cepstrum_189","cepstrum_190","cepstrum_191","cepstrum_192","cepstrum_193","cepstrum_194","cepstrum_195","cepstrum_196","cepstrum_197","cepstrum_198","cepstrum_199","cepstrum_200","cepstrum_201","cepstrum_202","cepstrum_203","cepstrum_204","cepstrum_205","cepstrum_206","cepstrum_207","cepstrum_208","cepstrum_209","cepstrum_210","cepstrum_211","cepstrum_212","cepstrum_213","cepstrum_214","cepstrum_215","cepstrum_216","cepstrum_217","cepstrum_218","cepstrum_219","cepstrum_220","cepstrum_221","cepstrum_222","cepstrum_223","cepstrum_224","cepstrum_225","cepstrum_226","cepstrum_227","cepstrum_228","cepstrum_229","cepstrum_230","cepstrum_231","cepstrum_232","cepstrum_233","cepstrum_234","cepstrum_235","cepstrum_236","cepstrum_237","cepstrum_238","cepstrum_239","cepstrum_240","cepstrum_241","cepstrum_242","cepstrum_243","cepstrum_244","cepstrum_245","cepstrum_246","cepstrum_247","cepstrum_248","cepstrum_249","cepstrum_250","cepstrum_251","cepstrum_252","cepstrum_253","cepstrum_254","cepstrum_255","cepstrum_256","cepstrum_257","cepstrum_258","cepstrum_259","cepstrum_260","cepstrum_261","cepstrum_262","cepstrum_263","cepstrum_264","cepstrum_265","cepstrum_266","cepstrum_267","cepstrum_268","cepstrum_269","cepstrum_270","cepstrum_271","cepstrum_272","cepstrum_273","cepstrum_274","cepstrum_275","cepstrum_276","cepstrum_277","cepstrum_278","cepstrum_279","cepstrum_280","cepstrum_281","cepstrum_282","cepstrum_283","cepstrum_284","cepstrum_285","cepstrum_286","cepstrum_287","cepstrum_288","cepstrum_289","cepstrum_290","cepstrum_291","cepstrum_292","cepstrum_293","cepstrum_294","cepstrum_295","cepstrum_296","cepstrum_297","cepstrum_298","cepstrum_299","cepstrum_300","cepstrum_301","cepstrum_302","cepstrum_303","cepstrum_304","cepstrum_305","cepstrum_306","cepstrum_307","cepstrum_308","cepstrum_309","cepstrum_310","cepstrum_311","cepstrum_312","cepstrum_313","cepstrum_314","cepstrum_315","cepstrum_316","cepstrum_317","cepstrum_318","cepstrum_319","cepstrum_320","cepstrum_321","cepstrum_322","cepstrum_323","cepstrum_324","cepstrum_325","cepstrum_326","cepstrum_327","cepstrum_328","cepstrum_329","cepstrum_330","cepstrum_331","cepstrum_332","cepstrum_333","cepstrum_334","cepstrum_335","cepstrum_336","cepstrum_337","cepstrum_338","cepstrum_339","cepstrum_340","cepstrum_341","cepstrum_342","cepstrum_343","cepstrum_344","cepstrum_345","cepstrum_346","cepstrum_347","cepstrum_348","cepstrum_349","cepstrum_350","cepstrum_351","cepstrum_352","cepstrum_353","mfcc_1","mfcc_2","mfcc_3","mfcc_4","mfcc_5","mfcc_6","mfcc_7","mfcc_8","mfcc_9","mfcc_10","mfcc_11","mfcc_12","mfcc_13","mfcc_14","mfcc_15","mfcc_16","mfcc_17","mfcc_18","mfcc_19","mfcc_20","mfcc_21","mfcc_22","mfcc_23","mfcc_24","mfcc_25","mfcc_26","mfcc_27","mfcc_28","mfcc_29","mfcc_30","mfcc_31","mfcc_32","mfcc_33","mfcc_34","mfcc_35","mfcc_36","mfcc_37","mfcc_38","peakSample_value","peakSample_index","zeroCrossing"};

    //============================= SAVE CSV ===================================
    // TODO: put stuff related to CSV save gere


    //============================== OTHER =====================================
    /**    Debugging    */
   #ifdef DEBUG_WHICH_CHANNEL
    uint32 logCounter = 0;  //To debug which channel to use
   #endif

    /**    Logging    **/
    /*
     * Since many log messages need to be written from the Audio thread, there
     * is a real-time safe logger that uses a FIFO queue as a ring buffer.

     * Messages are written to the queue with:
     * rtlogger.logInfo(message); // message is a char array of size RealTimeLogger::LogEntry::MESSAGE_LENGTH+1
     * or
     * rtlogger.logInfo(message, timeAtStart, timeAtEnd); // both times can be obtained with Time::getHighResolutionTicks();
     *
     * Messages have to be consumed with:
     * rtlogger.pop(le)
     *
     * Log Entries are consumed and written to file with a Juce FileLogger from a
     * low priority thread (timer callback), hence why a FileLogger is used.
    */
    tid::RealTimeLogger rtlogger{"main-app"}; // Real time logger that queues messages

    std::vector<tid::RealTimeLogger*> loggerList = {&rtlogger, aubioOnset.getLoggerPtr()};

   #ifdef DO_LOG_TO_FILE
    std::unique_ptr<FileLogger> fileLogger; // Logger that writes RT entries to file SAFELY (Outside rt thread
    std::string LOG_PATH = "/tmp/";
    std::string LOG_FILENAME = "featureExtractor";
    const int64 highResFrequency = Time::getHighResolutionTicksPerSecond();

    /**
     * Timer that calls a function every @interval milliseconds.
     * It is used to consume log entries and write them to file safely
    */
    typedef std::function<void (void)> TimerEventCallback;
    class PollingTimer : public Timer
    {
    public:
        PollingTimer(TimerEventCallback cb) { this->cb = cb; }
        void startLogRoutine() { Timer::startTimer(this->interval); }
        void startLogRoutine(int interval) { this->interval = interval; Timer::startTimer(this->interval); }
        void timerCallback() override { cb(); }
    private:
        TimerEventCallback cb;
        int interval = 500; // Half second default polling interval
    };

    /** POLLING ROUTINE (called at regular intervals by the timer) **/
    void logPollingRoutine()
    {
        /** INITIALIZE THE FILELOGGER IF NECESSARY **/
        if (!this->fileLogger)
            this->fileLogger = std::unique_ptr<FileLogger>(FileLogger::createDateStampedLogger(LOG_PATH, LOG_FILENAME, tIDLib::LOG_EXTENSION, "Feature Extractor"));

        tid::RealTimeLogger::LogEntry le;
        for(tid::RealTimeLogger* loggerpointer : this->loggerList) // Cycle through all loggers subscribed to the list
        {
            std::string moduleName = loggerpointer->getName();
            while (loggerpointer->pop(le))  // Continue if there are log entries in the rtlogger queue
            {
                /** CONSUME RTLOGGER ENTRIES AND WRITE THEM TO FILE **/
                std::string msg = moduleName + "\t|\t";
                msg += le.message;
                double start,end,diff;
                start = (le.timeAtStart * 1.0) / this->highResFrequency;
                end = (le.timeAtEnd * 1.0) / this->highResFrequency;
                diff = end-start;
                if (start != 0)
                {
                    msg += " | start: " + std::to_string(start);
                    msg += " | end: " + std::to_string(end);
                    msg += " | difference: " + std::to_string(diff) + "s";
                }
                fileLogger->logMessage(msg);
            }
        }
    }

    /** Instantiate polling timer and pass callback **/
    PollingTimer pollingTimer{[this]{logPollingRoutine();}};
   #endif



    /**    This atomic is used to update the onset LED in the GUI **/
    std::atomic<bool> onsetMonitorState{false};

    std::atomic<unsigned int> onsetCounterAtomic{0};

    enum class StorageState {
        idle,
        store
    };

    std::atomic<StorageState> storageState {StorageState::idle};


    // TODO: check atomic::wait
    // float oldOnsetMinIoi,oldOnsetThreshold,oldOnsetSilence;
    // std::atomic<float> onsetMinIoi,onsetThreshold,onsetSilence;
    std::atomic<bool> clearAt{false},writeAt{false};

    SaveToCsv<100000,VECTOR_SIZE,CSV_FLOAT_PRECISION> csvsaver;

   #ifdef LOG_LATENCY
    //Latency log

    int latencyCounter = 0,
        latencyLogPeriod = 1000;
   #endif









private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoProcessor)

public:
    //=========================== Juce System Stuff ============================
    DemoProcessor();
    ~DemoProcessor();
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif
    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
};
