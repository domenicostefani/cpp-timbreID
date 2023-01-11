/*
  ==============================================================================

  Feature Extractors
  Header that bungles a bunch of feature extractors from the TimbreID Puredata Externals

  Originally programmed by WBrent (https://github.com/wbrent/timbreIDLib)
  Ported to C++/JUCE by Domenico Stefani

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 11 January 2023

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "juce_timbreID.h"


namespace FE {

enum Extractor{ATTACKTIME,BARKSPECBRIGHTNESS,BARKSPEC,BFCC,CEPSTRUM,MFCC,PEAKSAMPLE,ZEROCROSSING};


/**
 * @brief Class for feature extractors.
 * @details  This class is a bungled collection of feature extractors from the TimbreID Puredata Externals
 * 
 */
template<short FE_WINDOW_SIZE>
class FeatureExtractors {
private:
    
    const bool USE_ATTACKTIME = true;
    const bool USE_BARKSPECBRIGHTNESS = true;
    const bool USE_BARKSPEC = true;
    const bool USE_BFCC = true;
    const bool USE_CEPSTRUM = true;
    const bool USE_MFCC = true;
    const bool USE_PEAKSAMPLE = true;
    const bool USE_ZEROCROSSING = true;

    static constexpr int _ATTACKTIME_RES_SIZE = 3;
    static constexpr int _BARKSPECBRIGHTNESS_RES_SIZE = 1;
    static constexpr int _BARKSPEC_SIZE = 50;
    static constexpr int _BFCC_RES_SIZE = 50;
    static constexpr int _CEPSTRUM_RES_SIZE = FE_WINDOW_SIZE/2+1; // WindowSize/2+1 (It would be 513 with 1025)
    static constexpr int _MFCC_RES_SIZE = 38;
    static constexpr int _PEAKSAMPLE_RES_SIZE = 2;
    static constexpr int _ZEROCROSSING_RES_SIZE = 1;

    //==================== FEATURE EXTRACTION PARAMS ===========================
    const unsigned int FEATUREEXT_WINDOW_SIZE = FE_WINDOW_SIZE; // 75 Blocks of 64samples, 100ms
    const float BARK_SPACING = 0.5f;
    const float BARK_BOUNDARY = 8.5f;
    const float MEL_SPACING = 100;

    //==================== FEATURE EXTRACTION OBJECTS ==========================
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

    const int ATTACKTIME_RES_SIZE = _ATTACKTIME_RES_SIZE;
    const int BARKSPECBRIGHTNESS_RES_SIZE = _BARKSPECBRIGHTNESS_RES_SIZE;
    const int BARKSPEC_SIZE = _BARKSPEC_SIZE;
    std::vector<float> barkSpecRes;
    const int BFCC_RES_SIZE = 50;
    std::vector<float> bfccRes;
    const int CEPSTRUM_RES_SIZE = _CEPSTRUM_RES_SIZE;
    std::vector<float> cepstrumRes;
    const int MFCC_RES_SIZE = _MFCC_RES_SIZE;
    std::vector<float> mfccRes;
    const int PEAKSAMPLE_RES_SIZE = _PEAKSAMPLE_RES_SIZE;
    const int ZEROCROSSING_RES_SIZE = _ZEROCROSSING_RES_SIZE;

    static const unsigned int VECTOR_SIZE = _ATTACKTIME_RES_SIZE+\
                                            _BARKSPECBRIGHTNESS_RES_SIZE+\
                                            _BARKSPEC_SIZE+\
                                            _BFCC_RES_SIZE+\
                                            _CEPSTRUM_RES_SIZE+\
                                            _MFCC_RES_SIZE+\
                                            _PEAKSAMPLE_RES_SIZE+\
                                            _ZEROCROSSING_RES_SIZE;

    #ifdef LONG_WINDOW                                      
    // This is for a window of 4800 samples
    const std::vector<std::string> header = {"attackTime_peaksamp","attackTime_attackStartIdx","attackTime_value",\
                                             "barkSpecBrightness",\
                                             "barkSpec_1","barkSpec_2","barkSpec_3","barkSpec_4","barkSpec_5","barkSpec_6","barkSpec_7","barkSpec_8","barkSpec_9","barkSpec_10","barkSpec_11","barkSpec_12","barkSpec_13","barkSpec_14","barkSpec_15","barkSpec_16","barkSpec_17","barkSpec_18","barkSpec_19","barkSpec_20","barkSpec_21","barkSpec_22","barkSpec_23","barkSpec_24","barkSpec_25","barkSpec_26","barkSpec_27","barkSpec_28","barkSpec_29","barkSpec_30","barkSpec_31","barkSpec_32","barkSpec_33","barkSpec_34","barkSpec_35","barkSpec_36","barkSpec_37","barkSpec_38","barkSpec_39","barkSpec_40","barkSpec_41","barkSpec_42","barkSpec_43","barkSpec_44","barkSpec_45","barkSpec_46","barkSpec_47","barkSpec_48","barkSpec_49","barkSpec_50",\
                                             "bfcc_1","bfcc_2","bfcc_3","bfcc_4","bfcc_5","bfcc_6","bfcc_7","bfcc_8","bfcc_9","bfcc_10","bfcc_11","bfcc_12","bfcc_13","bfcc_14","bfcc_15","bfcc_16","bfcc_17","bfcc_18","bfcc_19","bfcc_20","bfcc_21","bfcc_22","bfcc_23","bfcc_24","bfcc_25","bfcc_26","bfcc_27","bfcc_28","bfcc_29","bfcc_30","bfcc_31","bfcc_32","bfcc_33","bfcc_34","bfcc_35","bfcc_36","bfcc_37","bfcc_38","bfcc_39","bfcc_40","bfcc_41","bfcc_42","bfcc_43","bfcc_44","bfcc_45","bfcc_46","bfcc_47","bfcc_48","bfcc_49","bfcc_50",\
                                             "cepstrum_1","cepstrum_2","cepstrum_3","cepstrum_4","cepstrum_5","cepstrum_6","cepstrum_7","cepstrum_8","cepstrum_9","cepstrum_10","cepstrum_11","cepstrum_12","cepstrum_13","cepstrum_14","cepstrum_15","cepstrum_16","cepstrum_17","cepstrum_18","cepstrum_19","cepstrum_20","cepstrum_21","cepstrum_22","cepstrum_23","cepstrum_24","cepstrum_25","cepstrum_26","cepstrum_27","cepstrum_28","cepstrum_29","cepstrum_30","cepstrum_31","cepstrum_32","cepstrum_33","cepstrum_34","cepstrum_35","cepstrum_36","cepstrum_37","cepstrum_38","cepstrum_39","cepstrum_40","cepstrum_41","cepstrum_42","cepstrum_43","cepstrum_44","cepstrum_45","cepstrum_46","cepstrum_47","cepstrum_48","cepstrum_49","cepstrum_50","cepstrum_51","cepstrum_52","cepstrum_53","cepstrum_54","cepstrum_55","cepstrum_56","cepstrum_57","cepstrum_58","cepstrum_59","cepstrum_60","cepstrum_61","cepstrum_62","cepstrum_63","cepstrum_64","cepstrum_65","cepstrum_66","cepstrum_67","cepstrum_68","cepstrum_69","cepstrum_70","cepstrum_71","cepstrum_72","cepstrum_73","cepstrum_74","cepstrum_75","cepstrum_76","cepstrum_77","cepstrum_78","cepstrum_79","cepstrum_80","cepstrum_81","cepstrum_82","cepstrum_83","cepstrum_84","cepstrum_85","cepstrum_86","cepstrum_87","cepstrum_88","cepstrum_89","cepstrum_90","cepstrum_91","cepstrum_92","cepstrum_93","cepstrum_94","cepstrum_95","cepstrum_96","cepstrum_97","cepstrum_98","cepstrum_99","cepstrum_100","cepstrum_101","cepstrum_102","cepstrum_103","cepstrum_104","cepstrum_105","cepstrum_106","cepstrum_107","cepstrum_108","cepstrum_109","cepstrum_110","cepstrum_111","cepstrum_112","cepstrum_113","cepstrum_114","cepstrum_115","cepstrum_116","cepstrum_117","cepstrum_118","cepstrum_119","cepstrum_120","cepstrum_121","cepstrum_122","cepstrum_123","cepstrum_124","cepstrum_125","cepstrum_126","cepstrum_127","cepstrum_128","cepstrum_129","cepstrum_130","cepstrum_131","cepstrum_132","cepstrum_133","cepstrum_134","cepstrum_135","cepstrum_136","cepstrum_137","cepstrum_138","cepstrum_139","cepstrum_140","cepstrum_141","cepstrum_142","cepstrum_143","cepstrum_144","cepstrum_145","cepstrum_146","cepstrum_147","cepstrum_148","cepstrum_149","cepstrum_150","cepstrum_151","cepstrum_152","cepstrum_153","cepstrum_154","cepstrum_155","cepstrum_156","cepstrum_157","cepstrum_158","cepstrum_159","cepstrum_160","cepstrum_161","cepstrum_162","cepstrum_163","cepstrum_164","cepstrum_165","cepstrum_166","cepstrum_167","cepstrum_168","cepstrum_169","cepstrum_170","cepstrum_171","cepstrum_172","cepstrum_173","cepstrum_174","cepstrum_175","cepstrum_176","cepstrum_177","cepstrum_178","cepstrum_179","cepstrum_180","cepstrum_181","cepstrum_182","cepstrum_183","cepstrum_184","cepstrum_185","cepstrum_186","cepstrum_187","cepstrum_188","cepstrum_189","cepstrum_190","cepstrum_191","cepstrum_192","cepstrum_193","cepstrum_194","cepstrum_195","cepstrum_196","cepstrum_197","cepstrum_198","cepstrum_199","cepstrum_200","cepstrum_201","cepstrum_202","cepstrum_203","cepstrum_204","cepstrum_205","cepstrum_206","cepstrum_207","cepstrum_208","cepstrum_209","cepstrum_210","cepstrum_211","cepstrum_212","cepstrum_213","cepstrum_214","cepstrum_215","cepstrum_216","cepstrum_217","cepstrum_218","cepstrum_219","cepstrum_220","cepstrum_221","cepstrum_222","cepstrum_223","cepstrum_224","cepstrum_225","cepstrum_226","cepstrum_227","cepstrum_228","cepstrum_229","cepstrum_230","cepstrum_231","cepstrum_232","cepstrum_233","cepstrum_234","cepstrum_235","cepstrum_236","cepstrum_237","cepstrum_238","cepstrum_239","cepstrum_240","cepstrum_241","cepstrum_242","cepstrum_243","cepstrum_244","cepstrum_245","cepstrum_246","cepstrum_247","cepstrum_248","cepstrum_249","cepstrum_250","cepstrum_251","cepstrum_252","cepstrum_253","cepstrum_254","cepstrum_255","cepstrum_256","cepstrum_257","cepstrum_258","cepstrum_259","cepstrum_260","cepstrum_261","cepstrum_262","cepstrum_263","cepstrum_264","cepstrum_265","cepstrum_266","cepstrum_267","cepstrum_268","cepstrum_269","cepstrum_270","cepstrum_271","cepstrum_272","cepstrum_273","cepstrum_274","cepstrum_275","cepstrum_276","cepstrum_277","cepstrum_278","cepstrum_279","cepstrum_280","cepstrum_281","cepstrum_282","cepstrum_283","cepstrum_284","cepstrum_285","cepstrum_286","cepstrum_287","cepstrum_288","cepstrum_289","cepstrum_290","cepstrum_291","cepstrum_292","cepstrum_293","cepstrum_294","cepstrum_295","cepstrum_296","cepstrum_297","cepstrum_298","cepstrum_299","cepstrum_300","cepstrum_301","cepstrum_302","cepstrum_303","cepstrum_304","cepstrum_305","cepstrum_306","cepstrum_307","cepstrum_308","cepstrum_309","cepstrum_310","cepstrum_311","cepstrum_312","cepstrum_313","cepstrum_314","cepstrum_315","cepstrum_316","cepstrum_317","cepstrum_318","cepstrum_319","cepstrum_320","cepstrum_321","cepstrum_322","cepstrum_323","cepstrum_324","cepstrum_325","cepstrum_326","cepstrum_327","cepstrum_328","cepstrum_329","cepstrum_330","cepstrum_331","cepstrum_332","cepstrum_333","cepstrum_334","cepstrum_335","cepstrum_336","cepstrum_337","cepstrum_338","cepstrum_339","cepstrum_340","cepstrum_341","cepstrum_342","cepstrum_343","cepstrum_344","cepstrum_345","cepstrum_346","cepstrum_347","cepstrum_348","cepstrum_349","cepstrum_350","cepstrum_351","cepstrum_352","cepstrum_353","cepstrum_354","cepstrum_355","cepstrum_356","cepstrum_357","cepstrum_358","cepstrum_359","cepstrum_360","cepstrum_361","cepstrum_362","cepstrum_363","cepstrum_364","cepstrum_365","cepstrum_366","cepstrum_367","cepstrum_368","cepstrum_369","cepstrum_370","cepstrum_371","cepstrum_372","cepstrum_373","cepstrum_374","cepstrum_375","cepstrum_376","cepstrum_377","cepstrum_378","cepstrum_379","cepstrum_380","cepstrum_381","cepstrum_382","cepstrum_383","cepstrum_384","cepstrum_385","cepstrum_386","cepstrum_387","cepstrum_388","cepstrum_389","cepstrum_390","cepstrum_391","cepstrum_392","cepstrum_393","cepstrum_394","cepstrum_395","cepstrum_396","cepstrum_397","cepstrum_398","cepstrum_399","cepstrum_400","cepstrum_401","cepstrum_402","cepstrum_403","cepstrum_404","cepstrum_405","cepstrum_406","cepstrum_407","cepstrum_408","cepstrum_409","cepstrum_410","cepstrum_411","cepstrum_412","cepstrum_413","cepstrum_414","cepstrum_415","cepstrum_416","cepstrum_417","cepstrum_418","cepstrum_419","cepstrum_420","cepstrum_421","cepstrum_422","cepstrum_423","cepstrum_424","cepstrum_425","cepstrum_426","cepstrum_427","cepstrum_428","cepstrum_429","cepstrum_430","cepstrum_431","cepstrum_432","cepstrum_433","cepstrum_434","cepstrum_435","cepstrum_436","cepstrum_437","cepstrum_438","cepstrum_439","cepstrum_440","cepstrum_441","cepstrum_442","cepstrum_443","cepstrum_444","cepstrum_445","cepstrum_446","cepstrum_447","cepstrum_448","cepstrum_449","cepstrum_450","cepstrum_451","cepstrum_452","cepstrum_453","cepstrum_454","cepstrum_455","cepstrum_456","cepstrum_457","cepstrum_458","cepstrum_459","cepstrum_460","cepstrum_461","cepstrum_462","cepstrum_463","cepstrum_464","cepstrum_465","cepstrum_466","cepstrum_467","cepstrum_468","cepstrum_469","cepstrum_470","cepstrum_471","cepstrum_472","cepstrum_473","cepstrum_474","cepstrum_475","cepstrum_476","cepstrum_477","cepstrum_478","cepstrum_479","cepstrum_480","cepstrum_481","cepstrum_482","cepstrum_483","cepstrum_484","cepstrum_485","cepstrum_486","cepstrum_487","cepstrum_488","cepstrum_489","cepstrum_490","cepstrum_491","cepstrum_492","cepstrum_493","cepstrum_494","cepstrum_495","cepstrum_496","cepstrum_497","cepstrum_498","cepstrum_499","cepstrum_500","cepstrum_501","cepstrum_502","cepstrum_503","cepstrum_504","cepstrum_505","cepstrum_506","cepstrum_507","cepstrum_508","cepstrum_509","cepstrum_510","cepstrum_511","cepstrum_512","cepstrum_513","cepstrum_514","cepstrum_515","cepstrum_516","cepstrum_517","cepstrum_518","cepstrum_519","cepstrum_520","cepstrum_521","cepstrum_522","cepstrum_523","cepstrum_524","cepstrum_525","cepstrum_526","cepstrum_527","cepstrum_528","cepstrum_529","cepstrum_530","cepstrum_531","cepstrum_532","cepstrum_533","cepstrum_534","cepstrum_535","cepstrum_536","cepstrum_537","cepstrum_538","cepstrum_539","cepstrum_540","cepstrum_541","cepstrum_542","cepstrum_543","cepstrum_544","cepstrum_545","cepstrum_546","cepstrum_547","cepstrum_548","cepstrum_549","cepstrum_550","cepstrum_551","cepstrum_552","cepstrum_553","cepstrum_554","cepstrum_555","cepstrum_556","cepstrum_557","cepstrum_558","cepstrum_559","cepstrum_560","cepstrum_561","cepstrum_562","cepstrum_563","cepstrum_564","cepstrum_565","cepstrum_566","cepstrum_567","cepstrum_568","cepstrum_569","cepstrum_570","cepstrum_571","cepstrum_572","cepstrum_573","cepstrum_574","cepstrum_575","cepstrum_576","cepstrum_577","cepstrum_578","cepstrum_579","cepstrum_580","cepstrum_581","cepstrum_582","cepstrum_583","cepstrum_584","cepstrum_585","cepstrum_586","cepstrum_587","cepstrum_588","cepstrum_589","cepstrum_590","cepstrum_591","cepstrum_592","cepstrum_593","cepstrum_594","cepstrum_595","cepstrum_596","cepstrum_597","cepstrum_598","cepstrum_599","cepstrum_600","cepstrum_601","cepstrum_602","cepstrum_603","cepstrum_604","cepstrum_605","cepstrum_606","cepstrum_607","cepstrum_608","cepstrum_609","cepstrum_610","cepstrum_611","cepstrum_612","cepstrum_613","cepstrum_614","cepstrum_615","cepstrum_616","cepstrum_617","cepstrum_618","cepstrum_619","cepstrum_620","cepstrum_621","cepstrum_622","cepstrum_623","cepstrum_624","cepstrum_625","cepstrum_626","cepstrum_627","cepstrum_628","cepstrum_629","cepstrum_630","cepstrum_631","cepstrum_632","cepstrum_633","cepstrum_634","cepstrum_635","cepstrum_636","cepstrum_637","cepstrum_638","cepstrum_639","cepstrum_640","cepstrum_641","cepstrum_642","cepstrum_643","cepstrum_644","cepstrum_645","cepstrum_646","cepstrum_647","cepstrum_648","cepstrum_649","cepstrum_650","cepstrum_651","cepstrum_652","cepstrum_653","cepstrum_654","cepstrum_655","cepstrum_656","cepstrum_657","cepstrum_658","cepstrum_659","cepstrum_660","cepstrum_661","cepstrum_662","cepstrum_663","cepstrum_664","cepstrum_665","cepstrum_666","cepstrum_667","cepstrum_668","cepstrum_669","cepstrum_670","cepstrum_671","cepstrum_672","cepstrum_673","cepstrum_674","cepstrum_675","cepstrum_676","cepstrum_677","cepstrum_678","cepstrum_679","cepstrum_680","cepstrum_681","cepstrum_682","cepstrum_683","cepstrum_684","cepstrum_685","cepstrum_686","cepstrum_687","cepstrum_688","cepstrum_689","cepstrum_690","cepstrum_691","cepstrum_692","cepstrum_693","cepstrum_694","cepstrum_695","cepstrum_696","cepstrum_697","cepstrum_698","cepstrum_699","cepstrum_700","cepstrum_701","cepstrum_702","cepstrum_703","cepstrum_704","cepstrum_705","cepstrum_706","cepstrum_707","cepstrum_708","cepstrum_709","cepstrum_710","cepstrum_711","cepstrum_712","cepstrum_713","cepstrum_714","cepstrum_715","cepstrum_716","cepstrum_717","cepstrum_718","cepstrum_719","cepstrum_720","cepstrum_721","cepstrum_722","cepstrum_723","cepstrum_724","cepstrum_725","cepstrum_726","cepstrum_727","cepstrum_728","cepstrum_729","cepstrum_730","cepstrum_731","cepstrum_732","cepstrum_733","cepstrum_734","cepstrum_735","cepstrum_736","cepstrum_737","cepstrum_738","cepstrum_739","cepstrum_740","cepstrum_741","cepstrum_742","cepstrum_743","cepstrum_744","cepstrum_745","cepstrum_746","cepstrum_747","cepstrum_748","cepstrum_749","cepstrum_750","cepstrum_751","cepstrum_752","cepstrum_753","cepstrum_754","cepstrum_755","cepstrum_756","cepstrum_757","cepstrum_758","cepstrum_759","cepstrum_760","cepstrum_761","cepstrum_762","cepstrum_763","cepstrum_764","cepstrum_765","cepstrum_766","cepstrum_767","cepstrum_768","cepstrum_769","cepstrum_770","cepstrum_771","cepstrum_772","cepstrum_773","cepstrum_774","cepstrum_775","cepstrum_776","cepstrum_777","cepstrum_778","cepstrum_779","cepstrum_780","cepstrum_781","cepstrum_782","cepstrum_783","cepstrum_784","cepstrum_785","cepstrum_786","cepstrum_787","cepstrum_788","cepstrum_789","cepstrum_790","cepstrum_791","cepstrum_792","cepstrum_793","cepstrum_794","cepstrum_795","cepstrum_796","cepstrum_797","cepstrum_798","cepstrum_799","cepstrum_800","cepstrum_801","cepstrum_802","cepstrum_803","cepstrum_804","cepstrum_805","cepstrum_806","cepstrum_807","cepstrum_808","cepstrum_809","cepstrum_810","cepstrum_811","cepstrum_812","cepstrum_813","cepstrum_814","cepstrum_815","cepstrum_816","cepstrum_817","cepstrum_818","cepstrum_819","cepstrum_820","cepstrum_821","cepstrum_822","cepstrum_823","cepstrum_824","cepstrum_825","cepstrum_826","cepstrum_827","cepstrum_828","cepstrum_829","cepstrum_830","cepstrum_831","cepstrum_832","cepstrum_833","cepstrum_834","cepstrum_835","cepstrum_836","cepstrum_837","cepstrum_838","cepstrum_839","cepstrum_840","cepstrum_841","cepstrum_842","cepstrum_843","cepstrum_844","cepstrum_845","cepstrum_846","cepstrum_847","cepstrum_848","cepstrum_849","cepstrum_850","cepstrum_851","cepstrum_852","cepstrum_853","cepstrum_854","cepstrum_855","cepstrum_856","cepstrum_857","cepstrum_858","cepstrum_859","cepstrum_860","cepstrum_861","cepstrum_862","cepstrum_863","cepstrum_864","cepstrum_865","cepstrum_866","cepstrum_867","cepstrum_868","cepstrum_869","cepstrum_870","cepstrum_871","cepstrum_872","cepstrum_873","cepstrum_874","cepstrum_875","cepstrum_876","cepstrum_877","cepstrum_878","cepstrum_879","cepstrum_880","cepstrum_881","cepstrum_882","cepstrum_883","cepstrum_884","cepstrum_885","cepstrum_886","cepstrum_887","cepstrum_888","cepstrum_889","cepstrum_890","cepstrum_891","cepstrum_892","cepstrum_893","cepstrum_894","cepstrum_895","cepstrum_896","cepstrum_897","cepstrum_898","cepstrum_899","cepstrum_900","cepstrum_901","cepstrum_902","cepstrum_903","cepstrum_904","cepstrum_905","cepstrum_906","cepstrum_907","cepstrum_908","cepstrum_909","cepstrum_910","cepstrum_911","cepstrum_912","cepstrum_913","cepstrum_914","cepstrum_915","cepstrum_916","cepstrum_917","cepstrum_918","cepstrum_919","cepstrum_920","cepstrum_921","cepstrum_922","cepstrum_923","cepstrum_924","cepstrum_925","cepstrum_926","cepstrum_927","cepstrum_928","cepstrum_929","cepstrum_930","cepstrum_931","cepstrum_932","cepstrum_933","cepstrum_934","cepstrum_935","cepstrum_936","cepstrum_937","cepstrum_938","cepstrum_939","cepstrum_940","cepstrum_941","cepstrum_942","cepstrum_943","cepstrum_944","cepstrum_945","cepstrum_946","cepstrum_947","cepstrum_948","cepstrum_949","cepstrum_950","cepstrum_951","cepstrum_952","cepstrum_953","cepstrum_954","cepstrum_955","cepstrum_956","cepstrum_957","cepstrum_958","cepstrum_959","cepstrum_960","cepstrum_961","cepstrum_962","cepstrum_963","cepstrum_964","cepstrum_965","cepstrum_966","cepstrum_967","cepstrum_968","cepstrum_969","cepstrum_970","cepstrum_971","cepstrum_972","cepstrum_973","cepstrum_974","cepstrum_975","cepstrum_976","cepstrum_977","cepstrum_978","cepstrum_979","cepstrum_980","cepstrum_981","cepstrum_982","cepstrum_983","cepstrum_984","cepstrum_985","cepstrum_986","cepstrum_987","cepstrum_988","cepstrum_989","cepstrum_990","cepstrum_991","cepstrum_992","cepstrum_993","cepstrum_994","cepstrum_995","cepstrum_996","cepstrum_997","cepstrum_998","cepstrum_999","cepstrum_1000","cepstrum_1001","cepstrum_1002","cepstrum_1003","cepstrum_1004","cepstrum_1005","cepstrum_1006","cepstrum_1007","cepstrum_1008","cepstrum_1009","cepstrum_1010","cepstrum_1011","cepstrum_1012","cepstrum_1013","cepstrum_1014","cepstrum_1015","cepstrum_1016","cepstrum_1017","cepstrum_1018","cepstrum_1019","cepstrum_1020","cepstrum_1021","cepstrum_1022","cepstrum_1023","cepstrum_1024","cepstrum_1025","cepstrum_1026","cepstrum_1027","cepstrum_1028","cepstrum_1029","cepstrum_1030","cepstrum_1031","cepstrum_1032","cepstrum_1033","cepstrum_1034","cepstrum_1035","cepstrum_1036","cepstrum_1037","cepstrum_1038","cepstrum_1039","cepstrum_1040","cepstrum_1041","cepstrum_1042","cepstrum_1043","cepstrum_1044","cepstrum_1045","cepstrum_1046","cepstrum_1047","cepstrum_1048","cepstrum_1049","cepstrum_1050","cepstrum_1051","cepstrum_1052","cepstrum_1053","cepstrum_1054","cepstrum_1055","cepstrum_1056","cepstrum_1057","cepstrum_1058","cepstrum_1059","cepstrum_1060","cepstrum_1061","cepstrum_1062","cepstrum_1063","cepstrum_1064","cepstrum_1065","cepstrum_1066","cepstrum_1067","cepstrum_1068","cepstrum_1069","cepstrum_1070","cepstrum_1071","cepstrum_1072","cepstrum_1073","cepstrum_1074","cepstrum_1075","cepstrum_1076","cepstrum_1077","cepstrum_1078","cepstrum_1079","cepstrum_1080","cepstrum_1081","cepstrum_1082","cepstrum_1083","cepstrum_1084","cepstrum_1085","cepstrum_1086","cepstrum_1087","cepstrum_1088","cepstrum_1089","cepstrum_1090","cepstrum_1091","cepstrum_1092","cepstrum_1093","cepstrum_1094","cepstrum_1095","cepstrum_1096","cepstrum_1097","cepstrum_1098","cepstrum_1099","cepstrum_1100","cepstrum_1101","cepstrum_1102","cepstrum_1103","cepstrum_1104","cepstrum_1105","cepstrum_1106","cepstrum_1107","cepstrum_1108","cepstrum_1109","cepstrum_1110","cepstrum_1111","cepstrum_1112","cepstrum_1113","cepstrum_1114","cepstrum_1115","cepstrum_1116","cepstrum_1117","cepstrum_1118","cepstrum_1119","cepstrum_1120","cepstrum_1121","cepstrum_1122","cepstrum_1123","cepstrum_1124","cepstrum_1125","cepstrum_1126","cepstrum_1127","cepstrum_1128","cepstrum_1129","cepstrum_1130","cepstrum_1131","cepstrum_1132","cepstrum_1133","cepstrum_1134","cepstrum_1135","cepstrum_1136","cepstrum_1137","cepstrum_1138","cepstrum_1139","cepstrum_1140","cepstrum_1141","cepstrum_1142","cepstrum_1143","cepstrum_1144","cepstrum_1145","cepstrum_1146","cepstrum_1147","cepstrum_1148","cepstrum_1149","cepstrum_1150","cepstrum_1151","cepstrum_1152","cepstrum_1153","cepstrum_1154","cepstrum_1155","cepstrum_1156","cepstrum_1157","cepstrum_1158","cepstrum_1159","cepstrum_1160","cepstrum_1161","cepstrum_1162","cepstrum_1163","cepstrum_1164","cepstrum_1165","cepstrum_1166","cepstrum_1167","cepstrum_1168","cepstrum_1169","cepstrum_1170","cepstrum_1171","cepstrum_1172","cepstrum_1173","cepstrum_1174","cepstrum_1175","cepstrum_1176","cepstrum_1177","cepstrum_1178","cepstrum_1179","cepstrum_1180","cepstrum_1181","cepstrum_1182","cepstrum_1183","cepstrum_1184","cepstrum_1185","cepstrum_1186","cepstrum_1187","cepstrum_1188","cepstrum_1189","cepstrum_1190","cepstrum_1191","cepstrum_1192","cepstrum_1193","cepstrum_1194","cepstrum_1195","cepstrum_1196","cepstrum_1197","cepstrum_1198","cepstrum_1199","cepstrum_1200","cepstrum_1201","cepstrum_1202","cepstrum_1203","cepstrum_1204","cepstrum_1205","cepstrum_1206","cepstrum_1207","cepstrum_1208","cepstrum_1209","cepstrum_1210","cepstrum_1211","cepstrum_1212","cepstrum_1213","cepstrum_1214","cepstrum_1215","cepstrum_1216","cepstrum_1217","cepstrum_1218","cepstrum_1219","cepstrum_1220","cepstrum_1221","cepstrum_1222","cepstrum_1223","cepstrum_1224","cepstrum_1225","cepstrum_1226","cepstrum_1227","cepstrum_1228","cepstrum_1229","cepstrum_1230","cepstrum_1231","cepstrum_1232","cepstrum_1233","cepstrum_1234","cepstrum_1235","cepstrum_1236","cepstrum_1237","cepstrum_1238","cepstrum_1239","cepstrum_1240","cepstrum_1241","cepstrum_1242","cepstrum_1243","cepstrum_1244","cepstrum_1245","cepstrum_1246","cepstrum_1247","cepstrum_1248","cepstrum_1249","cepstrum_1250","cepstrum_1251","cepstrum_1252","cepstrum_1253","cepstrum_1254","cepstrum_1255","cepstrum_1256","cepstrum_1257","cepstrum_1258","cepstrum_1259","cepstrum_1260","cepstrum_1261","cepstrum_1262","cepstrum_1263","cepstrum_1264","cepstrum_1265","cepstrum_1266","cepstrum_1267","cepstrum_1268","cepstrum_1269","cepstrum_1270","cepstrum_1271","cepstrum_1272","cepstrum_1273","cepstrum_1274","cepstrum_1275","cepstrum_1276","cepstrum_1277","cepstrum_1278","cepstrum_1279","cepstrum_1280","cepstrum_1281","cepstrum_1282","cepstrum_1283","cepstrum_1284","cepstrum_1285","cepstrum_1286","cepstrum_1287","cepstrum_1288","cepstrum_1289","cepstrum_1290","cepstrum_1291","cepstrum_1292","cepstrum_1293","cepstrum_1294","cepstrum_1295","cepstrum_1296","cepstrum_1297","cepstrum_1298","cepstrum_1299","cepstrum_1300","cepstrum_1301","cepstrum_1302","cepstrum_1303","cepstrum_1304","cepstrum_1305","cepstrum_1306","cepstrum_1307","cepstrum_1308","cepstrum_1309","cepstrum_1310","cepstrum_1311","cepstrum_1312","cepstrum_1313","cepstrum_1314","cepstrum_1315","cepstrum_1316","cepstrum_1317","cepstrum_1318","cepstrum_1319","cepstrum_1320","cepstrum_1321","cepstrum_1322","cepstrum_1323","cepstrum_1324","cepstrum_1325","cepstrum_1326","cepstrum_1327","cepstrum_1328","cepstrum_1329","cepstrum_1330","cepstrum_1331","cepstrum_1332","cepstrum_1333","cepstrum_1334","cepstrum_1335","cepstrum_1336","cepstrum_1337","cepstrum_1338","cepstrum_1339","cepstrum_1340","cepstrum_1341","cepstrum_1342","cepstrum_1343","cepstrum_1344","cepstrum_1345","cepstrum_1346","cepstrum_1347","cepstrum_1348","cepstrum_1349","cepstrum_1350","cepstrum_1351","cepstrum_1352","cepstrum_1353","cepstrum_1354","cepstrum_1355","cepstrum_1356","cepstrum_1357","cepstrum_1358","cepstrum_1359","cepstrum_1360","cepstrum_1361","cepstrum_1362","cepstrum_1363","cepstrum_1364","cepstrum_1365","cepstrum_1366","cepstrum_1367","cepstrum_1368","cepstrum_1369","cepstrum_1370","cepstrum_1371","cepstrum_1372","cepstrum_1373","cepstrum_1374","cepstrum_1375","cepstrum_1376","cepstrum_1377","cepstrum_1378","cepstrum_1379","cepstrum_1380","cepstrum_1381","cepstrum_1382","cepstrum_1383","cepstrum_1384","cepstrum_1385","cepstrum_1386","cepstrum_1387","cepstrum_1388","cepstrum_1389","cepstrum_1390","cepstrum_1391","cepstrum_1392","cepstrum_1393","cepstrum_1394","cepstrum_1395","cepstrum_1396","cepstrum_1397","cepstrum_1398","cepstrum_1399","cepstrum_1400","cepstrum_1401","cepstrum_1402","cepstrum_1403","cepstrum_1404","cepstrum_1405","cepstrum_1406","cepstrum_1407","cepstrum_1408","cepstrum_1409","cepstrum_1410","cepstrum_1411","cepstrum_1412","cepstrum_1413","cepstrum_1414","cepstrum_1415","cepstrum_1416","cepstrum_1417","cepstrum_1418","cepstrum_1419","cepstrum_1420","cepstrum_1421","cepstrum_1422","cepstrum_1423","cepstrum_1424","cepstrum_1425","cepstrum_1426","cepstrum_1427","cepstrum_1428","cepstrum_1429","cepstrum_1430","cepstrum_1431","cepstrum_1432","cepstrum_1433","cepstrum_1434","cepstrum_1435","cepstrum_1436","cepstrum_1437","cepstrum_1438","cepstrum_1439","cepstrum_1440","cepstrum_1441","cepstrum_1442","cepstrum_1443","cepstrum_1444","cepstrum_1445","cepstrum_1446","cepstrum_1447","cepstrum_1448","cepstrum_1449","cepstrum_1450","cepstrum_1451","cepstrum_1452","cepstrum_1453","cepstrum_1454","cepstrum_1455","cepstrum_1456","cepstrum_1457","cepstrum_1458","cepstrum_1459","cepstrum_1460","cepstrum_1461","cepstrum_1462","cepstrum_1463","cepstrum_1464","cepstrum_1465","cepstrum_1466","cepstrum_1467","cepstrum_1468","cepstrum_1469","cepstrum_1470","cepstrum_1471","cepstrum_1472","cepstrum_1473","cepstrum_1474","cepstrum_1475","cepstrum_1476","cepstrum_1477","cepstrum_1478","cepstrum_1479","cepstrum_1480","cepstrum_1481","cepstrum_1482","cepstrum_1483","cepstrum_1484","cepstrum_1485","cepstrum_1486","cepstrum_1487","cepstrum_1488","cepstrum_1489","cepstrum_1490","cepstrum_1491","cepstrum_1492","cepstrum_1493","cepstrum_1494","cepstrum_1495","cepstrum_1496","cepstrum_1497","cepstrum_1498","cepstrum_1499","cepstrum_1500","cepstrum_1501","cepstrum_1502","cepstrum_1503","cepstrum_1504","cepstrum_1505","cepstrum_1506","cepstrum_1507","cepstrum_1508","cepstrum_1509","cepstrum_1510","cepstrum_1511","cepstrum_1512","cepstrum_1513","cepstrum_1514","cepstrum_1515","cepstrum_1516","cepstrum_1517","cepstrum_1518","cepstrum_1519","cepstrum_1520","cepstrum_1521","cepstrum_1522","cepstrum_1523","cepstrum_1524","cepstrum_1525","cepstrum_1526","cepstrum_1527","cepstrum_1528","cepstrum_1529","cepstrum_1530","cepstrum_1531","cepstrum_1532","cepstrum_1533","cepstrum_1534","cepstrum_1535","cepstrum_1536","cepstrum_1537","cepstrum_1538","cepstrum_1539","cepstrum_1540","cepstrum_1541","cepstrum_1542","cepstrum_1543","cepstrum_1544","cepstrum_1545","cepstrum_1546","cepstrum_1547","cepstrum_1548","cepstrum_1549","cepstrum_1550","cepstrum_1551","cepstrum_1552","cepstrum_1553","cepstrum_1554","cepstrum_1555","cepstrum_1556","cepstrum_1557","cepstrum_1558","cepstrum_1559","cepstrum_1560","cepstrum_1561","cepstrum_1562","cepstrum_1563","cepstrum_1564","cepstrum_1565","cepstrum_1566","cepstrum_1567","cepstrum_1568","cepstrum_1569","cepstrum_1570","cepstrum_1571","cepstrum_1572","cepstrum_1573","cepstrum_1574","cepstrum_1575","cepstrum_1576","cepstrum_1577","cepstrum_1578","cepstrum_1579","cepstrum_1580","cepstrum_1581","cepstrum_1582","cepstrum_1583","cepstrum_1584","cepstrum_1585","cepstrum_1586","cepstrum_1587","cepstrum_1588","cepstrum_1589","cepstrum_1590","cepstrum_1591","cepstrum_1592","cepstrum_1593","cepstrum_1594","cepstrum_1595","cepstrum_1596","cepstrum_1597","cepstrum_1598","cepstrum_1599","cepstrum_1600","cepstrum_1601","cepstrum_1602","cepstrum_1603","cepstrum_1604","cepstrum_1605","cepstrum_1606","cepstrum_1607","cepstrum_1608","cepstrum_1609","cepstrum_1610","cepstrum_1611","cepstrum_1612","cepstrum_1613","cepstrum_1614","cepstrum_1615","cepstrum_1616","cepstrum_1617","cepstrum_1618","cepstrum_1619","cepstrum_1620","cepstrum_1621","cepstrum_1622","cepstrum_1623","cepstrum_1624","cepstrum_1625","cepstrum_1626","cepstrum_1627","cepstrum_1628","cepstrum_1629","cepstrum_1630","cepstrum_1631","cepstrum_1632","cepstrum_1633","cepstrum_1634","cepstrum_1635","cepstrum_1636","cepstrum_1637","cepstrum_1638","cepstrum_1639","cepstrum_1640","cepstrum_1641","cepstrum_1642","cepstrum_1643","cepstrum_1644","cepstrum_1645","cepstrum_1646","cepstrum_1647","cepstrum_1648","cepstrum_1649","cepstrum_1650","cepstrum_1651","cepstrum_1652","cepstrum_1653","cepstrum_1654","cepstrum_1655","cepstrum_1656","cepstrum_1657","cepstrum_1658","cepstrum_1659","cepstrum_1660","cepstrum_1661","cepstrum_1662","cepstrum_1663","cepstrum_1664","cepstrum_1665","cepstrum_1666","cepstrum_1667","cepstrum_1668","cepstrum_1669","cepstrum_1670","cepstrum_1671","cepstrum_1672","cepstrum_1673","cepstrum_1674","cepstrum_1675","cepstrum_1676","cepstrum_1677","cepstrum_1678","cepstrum_1679","cepstrum_1680","cepstrum_1681","cepstrum_1682","cepstrum_1683","cepstrum_1684","cepstrum_1685","cepstrum_1686","cepstrum_1687","cepstrum_1688","cepstrum_1689","cepstrum_1690","cepstrum_1691","cepstrum_1692","cepstrum_1693","cepstrum_1694","cepstrum_1695","cepstrum_1696","cepstrum_1697","cepstrum_1698","cepstrum_1699","cepstrum_1700","cepstrum_1701","cepstrum_1702","cepstrum_1703","cepstrum_1704","cepstrum_1705","cepstrum_1706","cepstrum_1707","cepstrum_1708","cepstrum_1709","cepstrum_1710","cepstrum_1711","cepstrum_1712","cepstrum_1713","cepstrum_1714","cepstrum_1715","cepstrum_1716","cepstrum_1717","cepstrum_1718","cepstrum_1719","cepstrum_1720","cepstrum_1721","cepstrum_1722","cepstrum_1723","cepstrum_1724","cepstrum_1725","cepstrum_1726","cepstrum_1727","cepstrum_1728","cepstrum_1729","cepstrum_1730","cepstrum_1731","cepstrum_1732","cepstrum_1733","cepstrum_1734","cepstrum_1735","cepstrum_1736","cepstrum_1737","cepstrum_1738","cepstrum_1739","cepstrum_1740","cepstrum_1741","cepstrum_1742","cepstrum_1743","cepstrum_1744","cepstrum_1745","cepstrum_1746","cepstrum_1747","cepstrum_1748","cepstrum_1749","cepstrum_1750","cepstrum_1751","cepstrum_1752","cepstrum_1753","cepstrum_1754","cepstrum_1755","cepstrum_1756","cepstrum_1757","cepstrum_1758","cepstrum_1759","cepstrum_1760","cepstrum_1761","cepstrum_1762","cepstrum_1763","cepstrum_1764","cepstrum_1765","cepstrum_1766","cepstrum_1767","cepstrum_1768","cepstrum_1769","cepstrum_1770","cepstrum_1771","cepstrum_1772","cepstrum_1773","cepstrum_1774","cepstrum_1775","cepstrum_1776","cepstrum_1777","cepstrum_1778","cepstrum_1779","cepstrum_1780","cepstrum_1781","cepstrum_1782","cepstrum_1783","cepstrum_1784","cepstrum_1785","cepstrum_1786","cepstrum_1787","cepstrum_1788","cepstrum_1789","cepstrum_1790","cepstrum_1791","cepstrum_1792","cepstrum_1793","cepstrum_1794","cepstrum_1795","cepstrum_1796","cepstrum_1797","cepstrum_1798","cepstrum_1799","cepstrum_1800","cepstrum_1801","cepstrum_1802","cepstrum_1803","cepstrum_1804","cepstrum_1805","cepstrum_1806","cepstrum_1807","cepstrum_1808","cepstrum_1809","cepstrum_1810","cepstrum_1811","cepstrum_1812","cepstrum_1813","cepstrum_1814","cepstrum_1815","cepstrum_1816","cepstrum_1817","cepstrum_1818","cepstrum_1819","cepstrum_1820","cepstrum_1821","cepstrum_1822","cepstrum_1823","cepstrum_1824","cepstrum_1825","cepstrum_1826","cepstrum_1827","cepstrum_1828","cepstrum_1829","cepstrum_1830","cepstrum_1831","cepstrum_1832","cepstrum_1833","cepstrum_1834","cepstrum_1835","cepstrum_1836","cepstrum_1837","cepstrum_1838","cepstrum_1839","cepstrum_1840","cepstrum_1841","cepstrum_1842","cepstrum_1843","cepstrum_1844","cepstrum_1845","cepstrum_1846","cepstrum_1847","cepstrum_1848","cepstrum_1849","cepstrum_1850","cepstrum_1851","cepstrum_1852","cepstrum_1853","cepstrum_1854","cepstrum_1855","cepstrum_1856","cepstrum_1857","cepstrum_1858","cepstrum_1859","cepstrum_1860","cepstrum_1861","cepstrum_1862","cepstrum_1863","cepstrum_1864","cepstrum_1865","cepstrum_1866","cepstrum_1867","cepstrum_1868","cepstrum_1869","cepstrum_1870","cepstrum_1871","cepstrum_1872","cepstrum_1873","cepstrum_1874","cepstrum_1875","cepstrum_1876","cepstrum_1877","cepstrum_1878","cepstrum_1879","cepstrum_1880","cepstrum_1881","cepstrum_1882","cepstrum_1883","cepstrum_1884","cepstrum_1885","cepstrum_1886","cepstrum_1887","cepstrum_1888","cepstrum_1889","cepstrum_1890","cepstrum_1891","cepstrum_1892","cepstrum_1893","cepstrum_1894","cepstrum_1895","cepstrum_1896","cepstrum_1897","cepstrum_1898","cepstrum_1899","cepstrum_1900","cepstrum_1901","cepstrum_1902","cepstrum_1903","cepstrum_1904","cepstrum_1905","cepstrum_1906","cepstrum_1907","cepstrum_1908","cepstrum_1909","cepstrum_1910","cepstrum_1911","cepstrum_1912","cepstrum_1913","cepstrum_1914","cepstrum_1915","cepstrum_1916","cepstrum_1917","cepstrum_1918","cepstrum_1919","cepstrum_1920","cepstrum_1921","cepstrum_1922","cepstrum_1923","cepstrum_1924","cepstrum_1925","cepstrum_1926","cepstrum_1927","cepstrum_1928","cepstrum_1929","cepstrum_1930","cepstrum_1931","cepstrum_1932","cepstrum_1933","cepstrum_1934","cepstrum_1935","cepstrum_1936","cepstrum_1937","cepstrum_1938","cepstrum_1939","cepstrum_1940","cepstrum_1941","cepstrum_1942","cepstrum_1943","cepstrum_1944","cepstrum_1945","cepstrum_1946","cepstrum_1947","cepstrum_1948","cepstrum_1949","cepstrum_1950","cepstrum_1951","cepstrum_1952","cepstrum_1953","cepstrum_1954","cepstrum_1955","cepstrum_1956","cepstrum_1957","cepstrum_1958","cepstrum_1959","cepstrum_1960","cepstrum_1961","cepstrum_1962","cepstrum_1963","cepstrum_1964","cepstrum_1965","cepstrum_1966","cepstrum_1967","cepstrum_1968","cepstrum_1969","cepstrum_1970","cepstrum_1971","cepstrum_1972","cepstrum_1973","cepstrum_1974","cepstrum_1975","cepstrum_1976","cepstrum_1977","cepstrum_1978","cepstrum_1979","cepstrum_1980","cepstrum_1981","cepstrum_1982","cepstrum_1983","cepstrum_1984","cepstrum_1985","cepstrum_1986","cepstrum_1987","cepstrum_1988","cepstrum_1989","cepstrum_1990","cepstrum_1991","cepstrum_1992","cepstrum_1993","cepstrum_1994","cepstrum_1995","cepstrum_1996","cepstrum_1997","cepstrum_1998","cepstrum_1999","cepstrum_2000","cepstrum_2001","cepstrum_2002","cepstrum_2003","cepstrum_2004","cepstrum_2005","cepstrum_2006","cepstrum_2007","cepstrum_2008","cepstrum_2009","cepstrum_2010","cepstrum_2011","cepstrum_2012","cepstrum_2013","cepstrum_2014","cepstrum_2015","cepstrum_2016","cepstrum_2017","cepstrum_2018","cepstrum_2019","cepstrum_2020","cepstrum_2021","cepstrum_2022","cepstrum_2023","cepstrum_2024","cepstrum_2025","cepstrum_2026","cepstrum_2027","cepstrum_2028","cepstrum_2029","cepstrum_2030","cepstrum_2031","cepstrum_2032","cepstrum_2033","cepstrum_2034","cepstrum_2035","cepstrum_2036","cepstrum_2037","cepstrum_2038","cepstrum_2039","cepstrum_2040","cepstrum_2041","cepstrum_2042","cepstrum_2043","cepstrum_2044","cepstrum_2045","cepstrum_2046","cepstrum_2047","cepstrum_2048","cepstrum_2049","cepstrum_2050","cepstrum_2051","cepstrum_2052","cepstrum_2053","cepstrum_2054","cepstrum_2055","cepstrum_2056","cepstrum_2057","cepstrum_2058","cepstrum_2059","cepstrum_2060","cepstrum_2061","cepstrum_2062","cepstrum_2063","cepstrum_2064","cepstrum_2065","cepstrum_2066","cepstrum_2067","cepstrum_2068","cepstrum_2069","cepstrum_2070","cepstrum_2071","cepstrum_2072","cepstrum_2073","cepstrum_2074","cepstrum_2075","cepstrum_2076","cepstrum_2077","cepstrum_2078","cepstrum_2079","cepstrum_2080","cepstrum_2081","cepstrum_2082","cepstrum_2083","cepstrum_2084","cepstrum_2085","cepstrum_2086","cepstrum_2087","cepstrum_2088","cepstrum_2089","cepstrum_2090","cepstrum_2091","cepstrum_2092","cepstrum_2093","cepstrum_2094","cepstrum_2095","cepstrum_2096","cepstrum_2097","cepstrum_2098","cepstrum_2099","cepstrum_2100","cepstrum_2101","cepstrum_2102","cepstrum_2103","cepstrum_2104","cepstrum_2105","cepstrum_2106","cepstrum_2107","cepstrum_2108","cepstrum_2109","cepstrum_2110","cepstrum_2111","cepstrum_2112","cepstrum_2113","cepstrum_2114","cepstrum_2115","cepstrum_2116","cepstrum_2117","cepstrum_2118","cepstrum_2119","cepstrum_2120","cepstrum_2121","cepstrum_2122","cepstrum_2123","cepstrum_2124","cepstrum_2125","cepstrum_2126","cepstrum_2127","cepstrum_2128","cepstrum_2129","cepstrum_2130","cepstrum_2131","cepstrum_2132","cepstrum_2133","cepstrum_2134","cepstrum_2135","cepstrum_2136","cepstrum_2137","cepstrum_2138","cepstrum_2139","cepstrum_2140","cepstrum_2141","cepstrum_2142","cepstrum_2143","cepstrum_2144","cepstrum_2145","cepstrum_2146","cepstrum_2147","cepstrum_2148","cepstrum_2149","cepstrum_2150","cepstrum_2151","cepstrum_2152","cepstrum_2153","cepstrum_2154","cepstrum_2155","cepstrum_2156","cepstrum_2157","cepstrum_2158","cepstrum_2159","cepstrum_2160","cepstrum_2161","cepstrum_2162","cepstrum_2163","cepstrum_2164","cepstrum_2165","cepstrum_2166","cepstrum_2167","cepstrum_2168","cepstrum_2169","cepstrum_2170","cepstrum_2171","cepstrum_2172","cepstrum_2173","cepstrum_2174","cepstrum_2175","cepstrum_2176","cepstrum_2177","cepstrum_2178","cepstrum_2179","cepstrum_2180","cepstrum_2181","cepstrum_2182","cepstrum_2183","cepstrum_2184","cepstrum_2185","cepstrum_2186","cepstrum_2187","cepstrum_2188","cepstrum_2189","cepstrum_2190","cepstrum_2191","cepstrum_2192","cepstrum_2193","cepstrum_2194","cepstrum_2195","cepstrum_2196","cepstrum_2197","cepstrum_2198","cepstrum_2199","cepstrum_2200","cepstrum_2201","cepstrum_2202","cepstrum_2203","cepstrum_2204","cepstrum_2205","cepstrum_2206","cepstrum_2207","cepstrum_2208","cepstrum_2209","cepstrum_2210","cepstrum_2211","cepstrum_2212","cepstrum_2213","cepstrum_2214","cepstrum_2215","cepstrum_2216","cepstrum_2217","cepstrum_2218","cepstrum_2219","cepstrum_2220","cepstrum_2221","cepstrum_2222","cepstrum_2223","cepstrum_2224","cepstrum_2225","cepstrum_2226","cepstrum_2227","cepstrum_2228","cepstrum_2229","cepstrum_2230","cepstrum_2231","cepstrum_2232","cepstrum_2233","cepstrum_2234","cepstrum_2235","cepstrum_2236","cepstrum_2237","cepstrum_2238","cepstrum_2239","cepstrum_2240","cepstrum_2241","cepstrum_2242","cepstrum_2243","cepstrum_2244","cepstrum_2245","cepstrum_2246","cepstrum_2247","cepstrum_2248","cepstrum_2249","cepstrum_2250","cepstrum_2251","cepstrum_2252","cepstrum_2253","cepstrum_2254","cepstrum_2255","cepstrum_2256","cepstrum_2257","cepstrum_2258","cepstrum_2259","cepstrum_2260","cepstrum_2261","cepstrum_2262","cepstrum_2263","cepstrum_2264","cepstrum_2265","cepstrum_2266","cepstrum_2267","cepstrum_2268","cepstrum_2269","cepstrum_2270","cepstrum_2271","cepstrum_2272","cepstrum_2273","cepstrum_2274","cepstrum_2275","cepstrum_2276","cepstrum_2277","cepstrum_2278","cepstrum_2279","cepstrum_2280","cepstrum_2281","cepstrum_2282","cepstrum_2283","cepstrum_2284","cepstrum_2285","cepstrum_2286","cepstrum_2287","cepstrum_2288","cepstrum_2289","cepstrum_2290","cepstrum_2291","cepstrum_2292","cepstrum_2293","cepstrum_2294","cepstrum_2295","cepstrum_2296","cepstrum_2297","cepstrum_2298","cepstrum_2299","cepstrum_2300","cepstrum_2301","cepstrum_2302","cepstrum_2303","cepstrum_2304","cepstrum_2305","cepstrum_2306","cepstrum_2307","cepstrum_2308","cepstrum_2309","cepstrum_2310","cepstrum_2311","cepstrum_2312","cepstrum_2313","cepstrum_2314","cepstrum_2315","cepstrum_2316","cepstrum_2317","cepstrum_2318","cepstrum_2319","cepstrum_2320","cepstrum_2321","cepstrum_2322","cepstrum_2323","cepstrum_2324","cepstrum_2325","cepstrum_2326","cepstrum_2327","cepstrum_2328","cepstrum_2329","cepstrum_2330","cepstrum_2331","cepstrum_2332","cepstrum_2333","cepstrum_2334","cepstrum_2335","cepstrum_2336","cepstrum_2337","cepstrum_2338","cepstrum_2339","cepstrum_2340","cepstrum_2341","cepstrum_2342","cepstrum_2343","cepstrum_2344","cepstrum_2345","cepstrum_2346","cepstrum_2347","cepstrum_2348","cepstrum_2349","cepstrum_2350","cepstrum_2351","cepstrum_2352","cepstrum_2353","cepstrum_2354","cepstrum_2355","cepstrum_2356","cepstrum_2357","cepstrum_2358","cepstrum_2359","cepstrum_2360","cepstrum_2361","cepstrum_2362","cepstrum_2363","cepstrum_2364","cepstrum_2365","cepstrum_2366","cepstrum_2367","cepstrum_2368","cepstrum_2369","cepstrum_2370","cepstrum_2371","cepstrum_2372","cepstrum_2373","cepstrum_2374","cepstrum_2375","cepstrum_2376","cepstrum_2377","cepstrum_2378","cepstrum_2379","cepstrum_2380","cepstrum_2381","cepstrum_2382","cepstrum_2383","cepstrum_2384","cepstrum_2385","cepstrum_2386","cepstrum_2387","cepstrum_2388","cepstrum_2389","cepstrum_2390","cepstrum_2391","cepstrum_2392","cepstrum_2393","cepstrum_2394","cepstrum_2395","cepstrum_2396","cepstrum_2397","cepstrum_2398","cepstrum_2399","cepstrum_2400","cepstrum_2401",\
                                             "mfcc_1","mfcc_2","mfcc_3","mfcc_4","mfcc_5","mfcc_6","mfcc_7","mfcc_8","mfcc_9","mfcc_10","mfcc_11","mfcc_12","mfcc_13","mfcc_14","mfcc_15","mfcc_16","mfcc_17","mfcc_18","mfcc_19","mfcc_20","mfcc_21","mfcc_22","mfcc_23","mfcc_24","mfcc_25","mfcc_26","mfcc_27","mfcc_28","mfcc_29","mfcc_30","mfcc_31","mfcc_32","mfcc_33","mfcc_34","mfcc_35","mfcc_36","mfcc_37","mfcc_38",\
                                             "peakSample_value","peakSample_index",\
                                             "zeroCrossing"};
   #else
    // This is for a window of 704 samples
    const std::vector<std::string> header = {"attackTime_peaksamp","attackTime_attackStartIdx","attackTime_value",\
                                             "barkSpecBrightness",\
                                             "barkSpec_1","barkSpec_2","barkSpec_3","barkSpec_4","barkSpec_5","barkSpec_6","barkSpec_7","barkSpec_8","barkSpec_9","barkSpec_10","barkSpec_11","barkSpec_12","barkSpec_13","barkSpec_14","barkSpec_15","barkSpec_16","barkSpec_17","barkSpec_18","barkSpec_19","barkSpec_20","barkSpec_21","barkSpec_22","barkSpec_23","barkSpec_24","barkSpec_25","barkSpec_26","barkSpec_27","barkSpec_28","barkSpec_29","barkSpec_30","barkSpec_31","barkSpec_32","barkSpec_33","barkSpec_34","barkSpec_35","barkSpec_36","barkSpec_37","barkSpec_38","barkSpec_39","barkSpec_40","barkSpec_41","barkSpec_42","barkSpec_43","barkSpec_44","barkSpec_45","barkSpec_46","barkSpec_47","barkSpec_48","barkSpec_49","barkSpec_50",\
                                             "bfcc_1","bfcc_2","bfcc_3","bfcc_4","bfcc_5","bfcc_6","bfcc_7","bfcc_8","bfcc_9","bfcc_10","bfcc_11","bfcc_12","bfcc_13","bfcc_14","bfcc_15","bfcc_16","bfcc_17","bfcc_18","bfcc_19","bfcc_20","bfcc_21","bfcc_22","bfcc_23","bfcc_24","bfcc_25","bfcc_26","bfcc_27","bfcc_28","bfcc_29","bfcc_30","bfcc_31","bfcc_32","bfcc_33","bfcc_34","bfcc_35","bfcc_36","bfcc_37","bfcc_38","bfcc_39","bfcc_40","bfcc_41","bfcc_42","bfcc_43","bfcc_44","bfcc_45","bfcc_46","bfcc_47","bfcc_48","bfcc_49","bfcc_50",\
                                             "cepstrum_1","cepstrum_2","cepstrum_3","cepstrum_4","cepstrum_5","cepstrum_6","cepstrum_7","cepstrum_8","cepstrum_9","cepstrum_10","cepstrum_11","cepstrum_12","cepstrum_13","cepstrum_14","cepstrum_15","cepstrum_16","cepstrum_17","cepstrum_18","cepstrum_19","cepstrum_20","cepstrum_21","cepstrum_22","cepstrum_23","cepstrum_24","cepstrum_25","cepstrum_26","cepstrum_27","cepstrum_28","cepstrum_29","cepstrum_30","cepstrum_31","cepstrum_32","cepstrum_33","cepstrum_34","cepstrum_35","cepstrum_36","cepstrum_37","cepstrum_38","cepstrum_39","cepstrum_40","cepstrum_41","cepstrum_42","cepstrum_43","cepstrum_44","cepstrum_45","cepstrum_46","cepstrum_47","cepstrum_48","cepstrum_49","cepstrum_50","cepstrum_51","cepstrum_52","cepstrum_53","cepstrum_54","cepstrum_55","cepstrum_56","cepstrum_57","cepstrum_58","cepstrum_59","cepstrum_60","cepstrum_61","cepstrum_62","cepstrum_63","cepstrum_64","cepstrum_65","cepstrum_66","cepstrum_67","cepstrum_68","cepstrum_69","cepstrum_70","cepstrum_71","cepstrum_72","cepstrum_73","cepstrum_74","cepstrum_75","cepstrum_76","cepstrum_77","cepstrum_78","cepstrum_79","cepstrum_80","cepstrum_81","cepstrum_82","cepstrum_83","cepstrum_84","cepstrum_85","cepstrum_86","cepstrum_87","cepstrum_88","cepstrum_89","cepstrum_90","cepstrum_91","cepstrum_92","cepstrum_93","cepstrum_94","cepstrum_95","cepstrum_96","cepstrum_97","cepstrum_98","cepstrum_99","cepstrum_100","cepstrum_101","cepstrum_102","cepstrum_103","cepstrum_104","cepstrum_105","cepstrum_106","cepstrum_107","cepstrum_108","cepstrum_109","cepstrum_110","cepstrum_111","cepstrum_112","cepstrum_113","cepstrum_114","cepstrum_115","cepstrum_116","cepstrum_117","cepstrum_118","cepstrum_119","cepstrum_120","cepstrum_121","cepstrum_122","cepstrum_123","cepstrum_124","cepstrum_125","cepstrum_126","cepstrum_127","cepstrum_128","cepstrum_129","cepstrum_130","cepstrum_131","cepstrum_132","cepstrum_133","cepstrum_134","cepstrum_135","cepstrum_136","cepstrum_137","cepstrum_138","cepstrum_139","cepstrum_140","cepstrum_141","cepstrum_142","cepstrum_143","cepstrum_144","cepstrum_145","cepstrum_146","cepstrum_147","cepstrum_148","cepstrum_149","cepstrum_150","cepstrum_151","cepstrum_152","cepstrum_153","cepstrum_154","cepstrum_155","cepstrum_156","cepstrum_157","cepstrum_158","cepstrum_159","cepstrum_160","cepstrum_161","cepstrum_162","cepstrum_163","cepstrum_164","cepstrum_165","cepstrum_166","cepstrum_167","cepstrum_168","cepstrum_169","cepstrum_170","cepstrum_171","cepstrum_172","cepstrum_173","cepstrum_174","cepstrum_175","cepstrum_176","cepstrum_177","cepstrum_178","cepstrum_179","cepstrum_180","cepstrum_181","cepstrum_182","cepstrum_183","cepstrum_184","cepstrum_185","cepstrum_186","cepstrum_187","cepstrum_188","cepstrum_189","cepstrum_190","cepstrum_191","cepstrum_192","cepstrum_193","cepstrum_194","cepstrum_195","cepstrum_196","cepstrum_197","cepstrum_198","cepstrum_199","cepstrum_200","cepstrum_201","cepstrum_202","cepstrum_203","cepstrum_204","cepstrum_205","cepstrum_206","cepstrum_207","cepstrum_208","cepstrum_209","cepstrum_210","cepstrum_211","cepstrum_212","cepstrum_213","cepstrum_214","cepstrum_215","cepstrum_216","cepstrum_217","cepstrum_218","cepstrum_219","cepstrum_220","cepstrum_221","cepstrum_222","cepstrum_223","cepstrum_224","cepstrum_225","cepstrum_226","cepstrum_227","cepstrum_228","cepstrum_229","cepstrum_230","cepstrum_231","cepstrum_232","cepstrum_233","cepstrum_234","cepstrum_235","cepstrum_236","cepstrum_237","cepstrum_238","cepstrum_239","cepstrum_240","cepstrum_241","cepstrum_242","cepstrum_243","cepstrum_244","cepstrum_245","cepstrum_246","cepstrum_247","cepstrum_248","cepstrum_249","cepstrum_250","cepstrum_251","cepstrum_252","cepstrum_253","cepstrum_254","cepstrum_255","cepstrum_256","cepstrum_257","cepstrum_258","cepstrum_259","cepstrum_260","cepstrum_261","cepstrum_262","cepstrum_263","cepstrum_264","cepstrum_265","cepstrum_266","cepstrum_267","cepstrum_268","cepstrum_269","cepstrum_270","cepstrum_271","cepstrum_272","cepstrum_273","cepstrum_274","cepstrum_275","cepstrum_276","cepstrum_277","cepstrum_278","cepstrum_279","cepstrum_280","cepstrum_281","cepstrum_282","cepstrum_283","cepstrum_284","cepstrum_285","cepstrum_286","cepstrum_287","cepstrum_288","cepstrum_289","cepstrum_290","cepstrum_291","cepstrum_292","cepstrum_293","cepstrum_294","cepstrum_295","cepstrum_296","cepstrum_297","cepstrum_298","cepstrum_299","cepstrum_300","cepstrum_301","cepstrum_302","cepstrum_303","cepstrum_304","cepstrum_305","cepstrum_306","cepstrum_307","cepstrum_308","cepstrum_309","cepstrum_310","cepstrum_311","cepstrum_312","cepstrum_313","cepstrum_314","cepstrum_315","cepstrum_316","cepstrum_317","cepstrum_318","cepstrum_319","cepstrum_320","cepstrum_321","cepstrum_322","cepstrum_323","cepstrum_324","cepstrum_325","cepstrum_326","cepstrum_327","cepstrum_328","cepstrum_329","cepstrum_330","cepstrum_331","cepstrum_332","cepstrum_333","cepstrum_334","cepstrum_335","cepstrum_336","cepstrum_337","cepstrum_338","cepstrum_339","cepstrum_340","cepstrum_341","cepstrum_342","cepstrum_343","cepstrum_344","cepstrum_345","cepstrum_346","cepstrum_347","cepstrum_348","cepstrum_349","cepstrum_350","cepstrum_351","cepstrum_352","cepstrum_353",\
                                             "mfcc_1","mfcc_2","mfcc_3","mfcc_4","mfcc_5","mfcc_6","mfcc_7","mfcc_8","mfcc_9","mfcc_10","mfcc_11","mfcc_12","mfcc_13","mfcc_14","mfcc_15","mfcc_16","mfcc_17","mfcc_18","mfcc_19","mfcc_20","mfcc_21","mfcc_22","mfcc_23","mfcc_24","mfcc_25","mfcc_26","mfcc_27","mfcc_28","mfcc_29","mfcc_30","mfcc_31","mfcc_32","mfcc_33","mfcc_34","mfcc_35","mfcc_36","mfcc_37","mfcc_38",\
                                             "peakSample_value","peakSample_index",\
                                             "zeroCrossing"};
   #endif



public:

    // Constructor
    FeatureExtractors() {

    }

    constexpr static size_t getFeVectorSize() {
        return VECTOR_SIZE;
    }

    std::vector<std::string> getHeader() {
        return header;
    }

    void resizeVectors() {
        barkSpecRes.resize(BARKSPEC_SIZE);
        bfccRes.resize(BFCC_RES_SIZE);
        cepstrumRes.resize(CEPSTRUM_RES_SIZE);
        mfccRes.resize(MFCC_RES_SIZE);
        
        attackTime.setMaxSearchRange(20);
    }

    void prepare(double sampleRate, unsigned int samplesPerBlock) {
        /** Prepare feature extractors **/

        /*-------------------------------------/
        | Bark Frequency Cepstral Coefficients |
        /-------------------------------------*/
        bfcc.prepare(sampleRate, (uint32)samplesPerBlock);
        /*------------------------------------/
        | Cepstrum Coefficients               |
        /------------------------------------*/
        cepstrum.prepare(sampleRate, (uint32)samplesPerBlock);
        /*------------------------------------/
        | Attack time                         |
        /------------------------------------*/
        attackTime.prepare(sampleRate, (uint32)samplesPerBlock);
        /*------------------------------------/
        | Bark spectral brightness            |
        /------------------------------------*/
        barkSpecBrightness.prepare(sampleRate, (uint32)samplesPerBlock);
        /*------------------------------------/
        | Bark Spectrum                       |
        /------------------------------------*/
        barkSpec.prepare(sampleRate, (uint32)samplesPerBlock);
        /*------------------------------------/
        | Zero Crossings                      |
        /------------------------------------*/
        mfcc.prepare(sampleRate, (uint32)samplesPerBlock);
        /*------------------------------------/
        | Zero Crossings                      |
        /------------------------------------*/
        peakSample.prepare(sampleRate, (uint32)samplesPerBlock);
        /*------------------------------------/
        | Zero Crossings                      |
        /------------------------------------*/
        zeroCrossing.prepare(sampleRate, (uint32)samplesPerBlock);
    }

    void reset() {
        /*------------------------------------/
        | Reset the feature extractors        |
        /------------------------------------*/
        bfcc.reset();
        cepstrum.reset();
        attackTime.reset();
        barkSpecBrightness.reset();
        barkSpec.reset();
        mfcc.reset();
        peakSample.reset();
        zeroCrossing.reset();
    }

    // template <typename SampleType>
    // void store (AudioBuffer<OtherSampleType>& buffer, short channel)
    // {
    //     static_assert (std::is_same<OtherSampleType, SampleType>::value,
    //                    "The sample-type of the module must match the sample-type supplied to this store callback");
    template <typename SampleType>
    void store(AudioBuffer<SampleType>& buffer, short int channel) {
        bfcc.store(buffer,channel);
        cepstrum.store(buffer,channel);
        attackTime.store(buffer,channel);
        barkSpecBrightness.store(buffer,channel);
        barkSpec.store(buffer,channel);
        mfcc.store(buffer,channel);
        peakSample.store(buffer,channel);
        zeroCrossing.store(buffer,channel);
    }

    std::string getInfoString(FE::Extractor extractor) {
        switch (extractor)
        {
            case ATTACKTIME:
                return attackTime.getInfoString();
                break;
            case BARKSPECBRIGHTNESS:
                return barkSpecBrightness.getInfoString();
                break;
            case BARKSPEC:
                return barkSpec.getInfoString();
                break;
            case BFCC: 
                return bfcc.getInfoString();
                break;
            case CEPSTRUM:
                return cepstrum.getInfoString();
                break;
            case MFCC: 
                return mfcc.getInfoString();
                break;
            case PEAKSAMPLE:
                return peakSample.getInfoString();
                break;
            case ZEROCROSSING:
                return zeroCrossing.getInfoString();
                break;
            default:
                throw std::logic_error("Invalid Extractor");
        }
    }

    void computeFeatureVector(float featureVector[])
    {
        int last = -1;
        int newLast = 0;
        /*-----------------------------------------/
        | 01 - Attack time                         |
        /-----------------------------------------*/
        unsigned long int peakSampIdx = 0;
        unsigned long int attackStartIdx = 0;
        float attackTimeValue = 0.0f;
        this->attackTime.compute(&peakSampIdx, &attackStartIdx, &attackTimeValue);

        featureVector[0] = (float)peakSampIdx;
        featureVector[1] = (float)attackStartIdx;
        featureVector[2] = attackTimeValue;
        newLast = 2;
    #ifdef LOG_SIZES
        info += ("attackTime [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
    #endif
        last = newLast;

        /*-----------------------------------------/
        | 02 - Bark Spectral Brightness            |
        /-----------------------------------------*/
        float bsb = this->barkSpecBrightness.compute();

        featureVector[3] = bsb;
        newLast = 3;
    #ifdef LOG_SIZES
        info += ("barkSpecBrightness [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
    #endif
        last = newLast;

        /*-----------------------------------------/
        | 03 - Bark Spectrum                       |
        /-----------------------------------------*/
        barkSpecRes = this->barkSpec.compute();

        jassert(barkSpecRes.size() == BARKSPEC_SIZE);
        for(int i=0; i<BARKSPEC_SIZE; ++i)
        {
            featureVector[(last+1) + i] = barkSpecRes[i];
        }
        newLast = last + BARKSPEC_SIZE;
    #ifdef LOG_SIZES
        info += ("barkSpec [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
    #endif
        last = newLast;

        /*------------------------------------------/
        | 04 - Bark Frequency Cepstral Coefficients |
        /------------------------------------------*/
        bfccRes = this->bfcc.compute();
        jassert(bfccRes.size() == BFCC_RES_SIZE);
        for(int i=0; i<BFCC_RES_SIZE; ++i)
        {
            featureVector[(last+1) + i] = bfccRes[i];
        }
        newLast = last + BFCC_RES_SIZE;
    #ifdef LOG_SIZES
        info += ("bfcc [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
    #endif
        last = newLast;

        /*------------------------------------------/
        | 05 - Cepstrum Coefficients                |
        /------------------------------------------*/
        cepstrumRes = this->cepstrum.compute();
        jassert(cepstrumRes.size() == CEPSTRUM_RES_SIZE);
        for(int i=0; i<CEPSTRUM_RES_SIZE; ++i)
        {
            featureVector[(last+1) + i] = cepstrumRes[i];
        }
        newLast = last + CEPSTRUM_RES_SIZE;
    #ifdef LOG_SIZES
        info += ("cepstrum [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
    #endif
        last = newLast;

        /*-----------------------------------------/
        | 06 - Mel Frequency Cepstral Coefficients |
        /-----------------------------------------*/
        mfccRes = this->mfcc.compute();
        jassert(mfccRes.size() == MFCC_RES_SIZE);
        for(int i=0; i<MFCC_RES_SIZE; ++i)
        {
            featureVector[(last+1) + i] = mfccRes[i];
        }
        newLast = last + MFCC_RES_SIZE;
    #ifdef LOG_SIZES
        info += ("mfcc [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
    #endif
        last = newLast;

        /*-----------------------------------------/
        | 07 - Peak sample                         |
        /-----------------------------------------*/
        std::pair<float, unsigned long int> peakSample = this->peakSample.compute();
        float peakSampleRes = peakSample.first;
        unsigned long int peakSampleIndex = peakSample.second;
        featureVector[last+1] = peakSampleRes;
        featureVector[last+2] = peakSampleIndex;
        newLast = last + 2;
    #ifdef LOG_SIZES
        info += ("peakSample [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
    #endif
        last = newLast;

        /*-----------------------------------------/
        | 08 - Zero Crossings                      |
        /-----------------------------------------*/
        uint32 crossings = this->zeroCrossing.compute();
        featureVector[last+1] = crossings;
        newLast = last +1;
    #ifdef LOG_SIZES
        info += ("zeroCrossing [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
    #endif
        last = newLast;

    }

};

} 