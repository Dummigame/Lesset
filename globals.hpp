#ifndef LESSET
    #include "lesset.hpp"
#endif

namespace globals
{
    inline std::ostringstream oss;
    inline lessetB::Options options;

    inline std::unordered_map<std::string,std::string> userVariables;

    inline std::pair<std::vector<double>,std::vector<double>> points; 

    inline std::string ans;
    inline std::string errorMessage;
    inline std::string separator="'";
    inline bool error{};

    inline bool passedCalculationsFile{};
    inline bool passedInAsArg{};

    inline bool useDecimalComma{};

    inline int decimalPrecision{lessetB::defaultPrecision};
    #ifdef LESSET_INCLUDE_DEBUG
    inline bool debugCout{};
    inline bool debugCoutUsed{};
    #endif

    std::atomic<int> activeRecursions{};

    inline std::unordered_map<std::string, std::vector<lessetB::Token>> tokenMemory;
    
    const std::unordered_map<std::string, lessetB::tokenType> symbols
    {
        {"<<",    lessetB::tokenType::BINARYOP},
        {">>",    lessetB::tokenType::BINARYOP},
        {"B|",    lessetB::tokenType::BINARYOP},
        {"B!",    lessetB::tokenType::UNARYOP},
        {"~",    lessetB::tokenType::UNARYOP},
        {"NOT",    lessetB::tokenType::UNARYOP},
        {"&",    lessetB::tokenType::BINARYOP},
        {"^^",    lessetB::tokenType::BINARYOP},

        {"+",    lessetB::tokenType::BINARYOP},
        {"*",    lessetB::tokenType::BINARYOP},
        {"/",    lessetB::tokenType::BINARYOP},
        {"//",    lessetB::tokenType::BINARYOP},
        {":",    lessetB::tokenType::BINARYOP},
        {"^",    lessetB::tokenType::BINARYOP},
        {"%",    lessetB::tokenType::BINARYOP},
        {"<",    lessetB::tokenType::BINARYOP},
        {">",    lessetB::tokenType::BINARYOP},
        {"=",    lessetB::tokenType::BINARYOP},
        {"mod",    lessetB::tokenType::BINARYOP},
        {"fmod",    lessetB::tokenType::BINARYOP},
        {"rmod",    lessetB::tokenType::BINARYOP},
        {"nPk",    lessetB::tokenType::BINARYOP},
        {"nCk",    lessetB::tokenType::BINARYOP},
        {"**",    lessetB::tokenType::BINARYOP},
        {"AND",    lessetB::tokenType::BINARYOP},
        {"XOR",    lessetB::tokenType::BINARYOP},
        {"AROUND",    lessetB::tokenType::BINARYOP},
        {"OR",    lessetB::tokenType::BINARYOP},
        {"=!",    lessetB::tokenType::BINARYOP},
        {">=",    lessetB::tokenType::BINARYOP},
        {"<=",    lessetB::tokenType::BINARYOP},
        {"<=>", lessetB::tokenType::BINARYOP},

        {"!", lessetB::tokenType::UNARYOP},
        {"-", lessetB::tokenType::UNARYOP},
        {"!!", lessetB::tokenType::UNARYOP},

        {"pi", lessetB::tokenType::CONSTANT },
        {"i", lessetB::tokenType::CONSTANT },
        {"e", lessetB::tokenType::CONSTANT },
        {"a", lessetB::tokenType::CONSTANT },
        {"rnd", lessetB::tokenType::CONSTANT },
        {"rndint", lessetB::tokenType::CONSTANT },
        {"ec", lessetB::tokenType::CONSTANT },
        {"c", lessetB::tokenType::CONSTANT },
        {"R", lessetB::tokenType::CONSTANT },
        {"G", lessetB::tokenType::CONSTANT },
        {"g", lessetB::tokenType::CONSTANT },
        {"o", lessetB::tokenType::CONSTANT },
        {"h", lessetB::tokenType::CONSTANT },
        {"k", lessetB::tokenType::CONSTANT },
        {"H0", lessetB::tokenType::CONSTANT },
        {"Z0", lessetB::tokenType::CONSTANT },
        {"U0", lessetB::tokenType::CONSTANT },
        {"E0", lessetB::tokenType::CONSTANT },
        {"tau", lessetB::tokenType::CONSTANT },
        {"phi", lessetB::tokenType::CONSTANT },
        {"eul", lessetB::tokenType::CONSTANT },
        {"rad", lessetB::tokenType::CONSTANT },
        {"dgr", lessetB::tokenType::CONSTANT },
        {"inf", lessetB::tokenType::CONSTANT },
        {"nan", lessetB::tokenType::CONSTANT },
        {"ppm", lessetB::tokenType::CONSTANT },
        {"ppb", lessetB::tokenType::CONSTANT },
        {"ppt", lessetB::tokenType::CONSTANT },
        {"prc", lessetB::tokenType::CONSTANT },
        {"me", lessetB::tokenType::CONSTANT },
        {"ma", lessetB::tokenType::CONSTANT },
        {"Na", lessetB::tokenType::CONSTANT },
        {"true", lessetB::tokenType::CONSTANT },
        {"false", lessetB::tokenType::CONSTANT },
        {"ans", lessetB::tokenType::CONSTANT},
        {"x",lessetB::tokenType::VARIABLE},

        {"Re", lessetB::tokenType::FUNCTION},
        {"Im", lessetB::tokenType::FUNCTION},
        {"real", lessetB::tokenType::FUNCTION},
        {"imag", lessetB::tokenType::FUNCTION},
        {"arg", lessetB::tokenType::FUNCTION},
        {"norm",lessetB::tokenType::FUNCTION},
        {"conj",lessetB::tokenType::FUNCTION},
        {"proj",lessetB::tokenType::FUNCTION},
        // {"polar",lessetB::tokenType::FUNCTION},

        {"sinc", lessetB::tokenType::FUNCTION},
        {"sinc^2", lessetB::tokenType::FUNCTION},
        {"exp", lessetB::tokenType::FUNCTION},
        {"sign", lessetB::tokenType::FUNCTION},
        {"sqrt", lessetB::tokenType::FUNCTION},
        {"cbrt", lessetB::tokenType::FUNCTION},
        {"qtrt", lessetB::tokenType::FUNCTION},

        {"sin", lessetB::tokenType::FUNCTION},
        {"cos", lessetB::tokenType::FUNCTION},
        {"cis", lessetB::tokenType::FUNCTION},
        {"cas", lessetB::tokenType::FUNCTION},
        {"tan", lessetB::tokenType::FUNCTION},
        {"sinh", lessetB::tokenType::FUNCTION},
        {"cosh", lessetB::tokenType::FUNCTION},
        {"tanh", lessetB::tokenType::FUNCTION},

        {"asin", lessetB::tokenType::FUNCTION},
        {"acos", lessetB::tokenType::FUNCTION},
        {"atan", lessetB::tokenType::FUNCTION},
        {"asinh", lessetB::tokenType::FUNCTION},
        {"acosh", lessetB::tokenType::FUNCTION},
        {"atanh", lessetB::tokenType::FUNCTION},

        {"sin^-1", lessetB::tokenType::FUNCTION},
        {"cos^-1", lessetB::tokenType::FUNCTION},
        {"tan^-1", lessetB::tokenType::FUNCTION},
        {"sinh^-1", lessetB::tokenType::FUNCTION},
        {"cosh^-1", lessetB::tokenType::FUNCTION},
        {"tanh^-1", lessetB::tokenType::FUNCTION},

        {"sec", lessetB::tokenType::FUNCTION},
        {"csc", lessetB::tokenType::FUNCTION},
        {"cot", lessetB::tokenType::FUNCTION},
        {"sech", lessetB::tokenType::FUNCTION},
        {"csch", lessetB::tokenType::FUNCTION},
        {"coth", lessetB::tokenType::FUNCTION},

        {"sec^-1", lessetB::tokenType::FUNCTION},
        {"csc^-1", lessetB::tokenType::FUNCTION},
        {"cot^-1", lessetB::tokenType::FUNCTION},
        {"sech^-1", lessetB::tokenType::FUNCTION},
        {"csch^-1", lessetB::tokenType::FUNCTION}, //schhschhshuhuschush^-1
        {"coth^-1", lessetB::tokenType::FUNCTION},

        {"asec", lessetB::tokenType::FUNCTION},
        {"acsc", lessetB::tokenType::FUNCTION},
        {"acot", lessetB::tokenType::FUNCTION},
        {"asech", lessetB::tokenType::FUNCTION},
        {"acsch", lessetB::tokenType::FUNCTION}, //aschhschhshuhuschush
        {"acoth", lessetB::tokenType::FUNCTION},
        {"prime", lessetB::tokenType::FUNCTION},

        {"ln", lessetB::tokenType::FUNCTION},
        {"ln^2", lessetB::tokenType::FUNCTION},
        {"abs", lessetB::tokenType::FUNCTION},
        {"floor", lessetB::tokenType::FUNCTION},
        {"trunc", lessetB::tokenType::FUNCTION},
        {"ceil", lessetB::tokenType::FUNCTION},
        {"bround", lessetB::tokenType::FUNCTION},
        {"round", lessetB::tokenType::FUNCTION},
        {"sat", lessetB::tokenType::FUNCTION},
        {"ReLU", lessetB::tokenType::FUNCTION},
        {"sstep", lessetB::tokenType::FUNCTION},
        {"lgamma", lessetB::tokenType::FUNCTION},
        {"gamma", lessetB::tokenType::FUNCTION},

        {"sin^2", lessetB::tokenType::FUNCTION},
        {"cos^2", lessetB::tokenType::FUNCTION},
        {"cis^2", lessetB::tokenType::FUNCTION},
        {"cas^2", lessetB::tokenType::FUNCTION},
        {"tan^2", lessetB::tokenType::FUNCTION},
        {"sinh^2", lessetB::tokenType::FUNCTION},
        {"cosh^2", lessetB::tokenType::FUNCTION},
        {"tanh^2", lessetB::tokenType::FUNCTION},

        {"asin^2", lessetB::tokenType::FUNCTION},
        {"acos^2", lessetB::tokenType::FUNCTION},
        {"atan^2", lessetB::tokenType::FUNCTION},
        {"asinh^2", lessetB::tokenType::FUNCTION},
        {"acosh^2", lessetB::tokenType::FUNCTION},
        {"atanh^2", lessetB::tokenType::FUNCTION},

        {"sec^2", lessetB::tokenType::FUNCTION},
        {"csc^2", lessetB::tokenType::FUNCTION},
        {"cot^2", lessetB::tokenType::FUNCTION},
        {"sech^2", lessetB::tokenType::FUNCTION},
        {"csch^2", lessetB::tokenType::FUNCTION},
        {"coth^2", lessetB::tokenType::FUNCTION},

        {"asec^2", lessetB::tokenType::FUNCTION},
        {"acsc^2", lessetB::tokenType::FUNCTION},
        {"acot^2", lessetB::tokenType::FUNCTION},
        {"asech^2", lessetB::tokenType::FUNCTION},
        {"acsch^2", lessetB::tokenType::FUNCTION}, //aschhschhshuhuschush^2
        {"acoth^2", lessetB::tokenType::FUNCTION},

    };

    const std::unordered_map<std::string, lessetB::tokenType> multiArgFunctions
    {
        {"inRange", lessetB::tokenType::IN_RANGE},
        {"fromBin", lessetB::tokenType::FROM_BIN},
        {"fromHex", lessetB::tokenType::FROM_HEX},
        {"fromOct", lessetB::tokenType::FROM_OCT},
        {"root", lessetB::tokenType::ROOT},
        {"log", lessetB::tokenType::LOG},
        {"diff", lessetB::tokenType::DIFF},
        {"mean", lessetB::tokenType::MEAN},
        {"median", lessetB::tokenType::MEDIAN},
        {"stdevp", lessetB::tokenType::STDEVP},
        {"gcf", lessetB::tokenType::GCF},
        {"gcd", lessetB::tokenType::GCF},
        {"hcf", lessetB::tokenType::GCF},
        {"hcd", lessetB::tokenType::GCF},
        {"lcm", lessetB::tokenType::LCM},
        {"rndint", lessetB::tokenType::RNDINT},
        {"rndsel", lessetB::tokenType::RNDSEL},
        {"max", lessetB::tokenType::MAX},
        {"atan2", lessetB::tokenType::ATAN2},
        // {"smax", lessetB::tokenType::SMAX},
        {"min", lessetB::tokenType::MIN},
        // {"smin", lessetB::tokenType::SMIN},
        {"sabs", lessetB::tokenType::SABS},
        {"mix", lessetB::tokenType::MIX},
        {"if", lessetB::tokenType::IF},
        {"round", lessetB::tokenType::ROUND},
        {"trunc", lessetB::tokenType::TRUNC},
    };

    inline std::unordered_map<std::string, lessetB::Operation> userOperations
    {

    };

    inline std::unordered_map<std::string, lessetB::Operation> prevUserOperations
    {

    };

    inline std::unordered_map<std::string, lessetB::Function> userFunctions
    {

    };

    inline std::unordered_map<std::string, lessetB::Function> prevUserFunctions
    {

    };
    inline size_t functionCallID{};
    inline std::vector<std::unordered_map<std::string,std::string>> fnCallParams; // Stores arguments for function calls.

    const std::unordered_map<std::string, std::string> constants
    {
        {"nan" , "(nan,nan)"},
        {"e" , "(2.718281828459045235360287471352662497757247093699959574966967627724076630353547594571382178525166427427466391932003059921817413596629043572900334295260595630738132328627943490763233829880753195251019011573834187930702154089149934884167509244761460668,0)"},
        {"pi" , "(3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844609550582231725359408128481117450284102701938521105559644622948954930381964428810975665933446128475648233786783165271201909,0)"},
        {"tau" , "(6.283185307179586476925286766559005768394338798750211641949889184615632812572417997256069650684234135964296173026564613294187689219101164463450718816256962234900568205403877042211119289245897909860763928857621951331866892256951296467573566330542403818,0)"},
        {"phi" , "(1.618033988749894848204586834365638117720309179805762862135448622705260462818902449707207204189391137484754088075386891752126633862223536931793180060766726354433389086595939582905638322661319928290267880675208766892501711696207032221043216269548626297,0)"},
        {"eul" , "(0.57721566490153286060651209008240243104215933593992359880576723488486772677766467093694706329174674951463144724980708248096050401448654283622417399764492353625350033374293733773767394279259525824709491600873520394816567085323315177661152862119950150798,0)"},
        {"rad" , "(57.29577951308232087679815481410517033240547246656432154916024386120284714832155263244096899585111094418622338163286489328144826460124831503606826786341194212252638809746726792630798870289311076793826144263826315820961046048702050644425965684112017192,0)"},
        {"dgr" , "(0.01745329251994329576923690768488612713442871888541725456097191440171009114603449443682241569634509482212304492507379059248385469227528101239847421893404711731916824501501076956169755358123860530516878869127117208703296358960264249018770435091817334394,0)"},
        {"ppm" , "(0.000001,0)"},
        {"ppb" , "(0.000000001,0)"},
        {"ppt" , "(0.000000000001,0)"},
        {"prc" , "(0.01,0)"},
        {"c" , "(299792458,0)"},
        {"G" , "(6.6743e-11,0)"},
        {"g" , "(9.80665,0)"},
        {"o" , "(5.670374419e-08,0)"},
        {"k" , "(1.380649e-23,0)"},
        {"a" , "(0.0072973525693,0)"},
        {"h" , "(6.62607015e-34,0)"},
        {"i" , "(0,1)"}, // :o
        {"inf" , "(inf,0)"},
        {"true" , "(1,0)"},
        {"false" , "(0,0)"},
        {"H0" , "(2.2e-18,0)"},
        {"me" , "(5.9722e+24,0)"},
        {"ec" , "(1.602176634e-19,0)"},
        {"Z0" , "(376.730313668,0)"},
        {"U0" , "(1.25663706212e-06,0)"},
        {"E0" , "(8.8541878128e-12,0)"},
        {"ma" , "(1.6605390666e-27,0)"},
        {"R" , "(8.31446261815,0)"},
        {"Na" , "(6.02214076e+23,0)"},
        {"ans" , "ans"},
        {"rnd", "rnd"}, // These are replaced later
        {"rndint","rndint"},
    };
    
    const std::unordered_map<std::string, std::string> nonNumericalConstants
    {
    {"ans" , "ans"},
    {"rnd", "rnd"},
    {"rndint","rndint"},
    // {"inf" , "inf"},
    // {"nan" , "nan"},
    };

    const std::unordered_map<std::string, std::string> valueToConstant
    {
        { "1.414213562373095048801688724209698078569671875376948073176679737990732478462107038850387534327641573", "√2"},
        {"1.732050807568877293527446341505872366942805253810380628055806979451933016908800037081146186757248576", "√3"},
        { "2.718281828459045235360287471352662497757247093699959574966967627724076630353547594571382178525166427","ℯ"},
        { "3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117068","π"},
        // { "6.283185307179586476925286766559005768394338798750211641949889184615632812572417997256069650684234136","τ"},
        // { "1.618033988749894848204586834365638117720309179805762862135448622705260462818902449707207204189391137","φ"},
        // { "0.5772156649015328606065120900824024310421593359399235988057672348848677267776646709369470632917467495","γ"},
        // { "299792458","c"},
        // { "6.6743e-11","G"},
        // { "9.80665","g"},
        // {"5.670374419e-08","o" },
        // {"1.380649e-23","k" },
        // {"0.0072973525693","α" },
        // {"6.62607015e-34","h" },
        {"inf","∞" },
        {"-inf","-∞" },
        // {"2.2e-18","H0" },
        // {"5.9722e+24","me" },
        // {"1.602176634e-19","ec" },
        // {"376.730313668","Z0" },
        // {"1.25663706212e-06","U0" },
        // {"8.8541878128e-12","E0" },
        // {"1.6605390666e-27","ma" },
        // {"8.31446261815","R" },
        // {"6.02214076e+23","Na" },
        {"Not a Number","Not a Number"},
        {"nan","Not a Number"},
        {"true","true"},
        {"false","false"},
    };

    const std::unordered_map<std::string,std::string> asciiToUnicode
    {
        {"B!","~"},
        {"NOT","¬"},
        {"<=" , "≤"},
        {"a" , "α"},
        {"tau" , "τ"},
        {"rnd" , "ξ"},
        {"inf" , "∞"},
        {"phi" , "φ"},
        {"sqrt" , "√"},
        {"cbrt" , "∛"},
        {"qtrt" , "∜"},
        {"XOR" , "⊕"},
        {"OR" , "∨"},
        {"AND" , "∧"},
        {">=" , "≥"},
        {"=!" , "≠"},
        {"*" , "×"},
        {"pi" , "π"},
        {"eul" , "γ"},
        // {"e" , "ℯ"},
        {"AROUND" , "≈"},
    };

    const std::unordered_map<std::string, std::string> valueToConstantLenient
    {
        { "1.42", "√2"},
        {"1.73", "√3"},
        { "2.71","ℯ"},
        { "3.14","π"},
        { "1.57","π/2"},
        { "6.28","τ"},
        { "1.62","φ"},
        { "0.58","γ"},
        { "2.9979e+8","c"},
        { "6.6743e-11","G"},
        { "9.81","g"},
        {"5.670374419e-08","o" },
        {"1.380649e-23","k" },
        {"6.62607015e-34","h" },
        {"inf","∞" },
        {"-inf","-∞" },
        {"2.2e-18","H0" },
        {"5.9722e+24","me" },
        {"1.602176634e-19","ec" },
        {"376.73","Z0" },
        {"1.25663706212e-06","U0" },
        {"8.8541878128e-12","E0" },
        {"1.6605390666e-27","ma" },
        {"8.31","R" },
        {"6.02214076e+23","Na" },
        {"Not a Number","Not a Number"},
        {"nan","Not a Number"},
        {"true","true"},
        {"false","false"},
    };
    

    size_t threadCount{std::thread::hardware_concurrency()};

    const std::unordered_map<std::string, lessetB::pass> opToPriority
    {
        {"NOT",lessetB::NOT},
        {"B!",lessetB::NOT},
        {"<=>", lessetB::SPACESHIP},
        {"<<",    lessetB::BITWISE0},
        {">>",    lessetB::BITWISE0},
        {"B|",    lessetB::BITWISE1},
        {"&",    lessetB::BITWISE1},
        {"~",    lessetB::NOT},
        {"^^",    lessetB::BITWISE2},
        {"+",    lessetB::ADDITION},
        {"*",    lessetB::MULTIPLICATION},
        {"//",    lessetB::MULTIPLICATION},
        {"/",    lessetB::MULTIPLICATION},
        {":",    lessetB::MULTIPLICATION},
        {"h*",    lessetB::MULTIPLICATION_IMPLICIT},
        {"^",    lessetB::EXPONENTIATION},
        {"%",    lessetB::MULTIPLICATION},
        {"<",    lessetB::COMPARISONS1},
        {">",    lessetB::COMPARISONS2},
        {"=",    lessetB::COMPARISONS2},
        {"=!",    lessetB::COMPARISONS2},
        {"AROUND",    lessetB::COMPARISONS2},
        {"mod",    lessetB::MULTIPLICATION},
        {"fmod",    lessetB::MULTIPLICATION},
        {"rmod",    lessetB::MULTIPLICATION},
        {"nPk",    lessetB::MULTIPLICATION},
        {"nCk",    lessetB::MULTIPLICATION},
        {"**",    lessetB::EXPONENTIATION},
        {"AND",    lessetB::LOGIC1},
        {"XOR",    lessetB::LOGIC2},
        {"OR",    lessetB::LOGIC3},
        
        {">=",    lessetB::COMPARISONS1},
        {"<=",    lessetB::COMPARISONS1},
        {"!", lessetB::UNARY_OPS},
        {"!!", lessetB::UNARY_OPS},
        {"-", lessetB::UNARY_MINUS},
    };


    enum class idToOp
    {
        ADD=0,
        MULTIPLY=1,
        DIVIDE=2,
        EXPONENTIATE=4,
        MODULUS=5,
        LESS=6,
        GREATER=7,
        EQUALS=8,
        FMOD=10,
        RMOD=11,
        NPK=12,
        NCK=13,
        AND=15,
        XOR=16,
        AROUND=17,
        NOR=18,
        OR=19,
        NOT_EQ=20,
        GREATER_EQ=21,
        LESS_EQ=22,
        FACTORIAL=23,
        MINUS=24,
        DOUBLE_FACTORIAL=25,
        SINC=26,
        SINC_SQUARED=27,
        EXP=28,
        SIGN=29,
        SQRT=30,
        CBRT=31,
        QTRT=32,
        SIN=33,
        COS=34,
        TAN=35,
        SINH=36,
        COSH=37,
        TANH=38,
        ASIN=39,
        ACOS=40,
        ATAN=41,
        ASINH=42,
        ACOSH=43,
        ATANH=44,

        SEC=45,
        CSC=46,
        COT=47,

        SECH=48,
        CSCH=49,
        COTH=50,
        
        ASEC=51,
        ACSC=52,
        ACOT=53,
        ASECH=54,
        ACSCH=55, //ASCCSCHHSCUSCHUSH
        ACOTH=56,
        PRIME=57,

        LN=58,
        LN_SQUARED=59,
        ABS=60,
        FLOOR=61,
        TRUNC=62,
        CEIL=63,
        BANKERS_ROUND=64,
        ROUND=65,
        SAT=66,
        RELU=66+1,
        SSTEP=68,
        LGAMMA=69,
        GAMMA=70,

        SIN_SQUARED=71,
        COS_SQUARED=72,
        TAN_SQUARED=73,

        SINH_SQUARED=74,
        COSH_SQUARED=75,
        TANH_SQUARED=76,

        ASIN_SQUARED=77,
        ACOS_SQUARED=78,
        ATAN_SQUARED=79,

        ASINH_SQUARED=80,
        ACOSH_SQUARED=81,
        ATANH_SQUARED=82,

        SEC_SQUARED=83,
        CSC_SQUARED=84,
        COT_SQUARED=85,

        SECH_SQUARED=86,
        CSCH_SQUARED=87,
        COTH_SQUARED=88,

        ASEC_SQUARED=89,
        ACSC_SQUARED=90,
        ACOT_SQUARED=91,

        ASECH_SQUARED=92,
        ACSCH_SQUARED=93, //ASCHHSCHHSHUHUSCHUSH^2
        ACOTH_SQUARED=94,

        REAL=95,
        IMAG=96,

        ARG=97,
        NORM=98,
        CONJ=99,
        PROJ=100,

        CIS=101,
        CIS_SQUARED=102,
        CAS=103,
        CAS_SQUARED=104,

        FLOOR_DIV=105,

        SHIFT_RIGHT,
        SHIFT_LEFT,
        BITWISE_AND,
        BITWISE_OR,
        BITWISE_XOR,
        BITWISE_NOT,
        SPACESHIP,
        NOT,


    };

    
    using enum idToOp;
    const std::unordered_map<std::string, idToOp> opToID
    {
        {"~", BITWISE_NOT},
        {"B!", BITWISE_NOT},
        {"NOT", NOT},
        {"<=>", SPACESHIP},
        {">>", SHIFT_RIGHT},
        {"<<", SHIFT_LEFT},
        {"&", BITWISE_AND},
        {"B|",BITWISE_OR},
        {"^^",BITWISE_XOR},
        {"+",    ADD},
        {"*",    MULTIPLY},
        {"/",    DIVIDE},
        {":",    DIVIDE},
        {"h*",    MULTIPLY},
        {"^",    EXPONENTIATE},
        {"%",    MODULUS},
        {"<",    LESS},
        {">",    GREATER},
        {"=",    EQUALS},
        {"mod",    MODULUS},
        {"fmod",    FMOD},
        {"rmod",    RMOD},
        {"nPk",    NPK},
        {"nCk",    NCK},
        {"**",    EXPONENTIATE},
        {"AND",    AND},
        {"XOR",    XOR},
        {"AROUND",    AROUND},
        {"OR",    OR},
        {"=!",    NOT_EQ},
        {">=",    GREATER_EQ},
        {"<=",    LESS_EQ},

        {"!", FACTORIAL},
        {"-", MINUS},
        {"!!", DOUBLE_FACTORIAL},

        {"sinc", SINC},
        {"sinc^2", SINC_SQUARED},
        {"exp", EXP},
        {"sign", SIGN},
        {"sqrt", SQRT},
        {"cbrt", CBRT},
        {"qtrt", QTRT},

        {"sin", SIN},
        {"cos", COS},
        {"tan", TAN},
        {"sinh", SINH},
        {"cosh", COSH},
        {"tanh", TANH},

        {"asin", ASIN},
        {"acos", ACOS},
        {"atan", ATAN},
        {"asinh", ASINH},
        {"acosh", ACOSH},
        {"atanh", ATANH},

        {"sin^-1", ASIN},
        {"cos^-1", ACOS},
        {"tan^-1", ATAN},
        {"sinh^-1", ASINH},
        {"cosh^-1", ACOSH},
        {"tanh^-1", ATANH},

        {"sec", SEC},
        {"csc", CSC},
        {"cot", COT},
        {"sech", SECH},
        {"csch", CSCH},
        {"coth", COTH},

        {"asec", ASEC},
        {"acsc", ACSC},
        {"acot", ACOT},
        {"asech", ASECH},
        {"acsch", ACSCH}, //aschhschhshuhuschush
        {"acoth", ACOTH},
        {"prime", PRIME},

        {"sec^-1", ASEC},
        {"csc^-1", ACSC},
        {"cot^-1", ACOT},
        {"sech^-1", ASECH},
        {"csch^-1", ACSCH}, //schhschhshuhuschush^-1
        {"coth^-1", ACOTH},

        {"ln", LN},
        {"ln^2", LN_SQUARED},
        {"abs", ABS},
        {"floor", FLOOR},
        {"trunc", TRUNC},
        {"ceil", CEIL},
        {"bround", BANKERS_ROUND},
        {"round", ROUND},
        {"sat", SAT},
        {"ReLU", RELU},
        {"sstep", SSTEP},
        {"lgamma", LGAMMA},
        {"gamma", GAMMA},

        {"sin^2", SIN_SQUARED},
        {"cos^2", COS_SQUARED},
        {"tan^2", TAN_SQUARED},
        {"sinh^2", SINH_SQUARED},
        {"cosh^2", COSH_SQUARED},
        {"tanh^2", TANH_SQUARED},

        {"asin^2", ASIN_SQUARED},
        {"acos^2", ACOS_SQUARED},
        {"atan^2", ATAN_SQUARED},
        {"asinh^2", ASINH_SQUARED},
        {"acosh^2", ACOSH_SQUARED},
        {"atanh^2", ATANH_SQUARED},

        {"sec^2", SEC_SQUARED},
        {"csc^2", CSC_SQUARED},
        {"cot^2", COT_SQUARED},
        {"sech^2", SECH_SQUARED},
        {"csch^2", CSCH_SQUARED},
        {"coth^2", COTH_SQUARED},

        {"asec^2", ASEC_SQUARED},
        {"acsc^2", ACSC_SQUARED},
        {"acot^2", ACOT_SQUARED},
        {"asech^2", ASECH_SQUARED},
        {"acsch^2", ACSCH_SQUARED}, //aschhschhshuhuschush^2
        {"acoth^2", ACOTH_SQUARED},
    
        {"Re", REAL},
        {"Im", IMAG},  
        {"real", REAL},
        {"imag", IMAG},    
        {"arg", ARG},  
        {"norm",NORM},
        {"conj",CONJ},
        {"proj",PROJ},
        {"cis",CIS},
        {"cis^2",CIS_SQUARED},
        {"cas",CAS},
        {"cas^2",CAS_SQUARED},
        {"//",FLOOR_DIV},
        // {"polar",101}  

    };

}; // Namespace globals