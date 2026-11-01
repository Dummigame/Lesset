
#ifdef LESSET
    #error "Only include lesset.hpp once."
#endif

#define LESSET

#include <complex>
#include <limits>
#include <cctype>
#include <random>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <boost/math/constants/constants.hpp>
#include <sstream>
#include <boost/multiprecision/cpp_complex.hpp>
#include <boost/math/ccmath/fmod.hpp>
#include <type_traits>
#include <unordered_map>
#include <future>
struct Instance;

namespace lessetB
{
    struct CustomSymbols;
}

using graphNumType = double;
using graphNum = std::complex<graphNumType>;

inline bool containsX(const std::string &input, lessetB::CustomSymbols &customSymbols);
namespace lessetB
{
    
bool isValidInput(char);
inline graphNum sscanfWrapper(const std::string &s)
{
    graphNumType re{},im{};
    int successfullyConverted = sscanf(s.c_str(),"(%lf,%lf", &re,&im);
    assert(successfullyConverted==2);
    return graphNum(re,im);

}

inline graphNum sscanfWrapper(const std::string &s, int &converted)
{
    graphNumType re{},im{};
    converted=sscanf(s.c_str(),"(%lf,%lf", &re,&im);
    return graphNum(re,im);

}

using namespace boost::multiprecision;

inline std::random_device randev;
inline std::mt19937 randomMt(randev());

constexpr int maxPrecision = 100;     // Really powerful, upper limit for precision in calculations. Must be >=16 I think. Constants have up to 100 decimal places.
constexpr int defaultPrecision = 20;  // What is shown by default. Should not be higher than max.

static_assert(maxPrecision>=defaultPrecision, "Default precision should not exceed maximum");

#define MAX_KEYWORD_LENGTH 33 // Also limits custom keywords

enum pass : char8_t
{
    INITIAL,
    INVALID,

    SUBEXPRESSIONS,
    CS1,
    FUNCTIONS,
    CS2,
    UNARY_OPS,
    CS3,
    EXPONENTIATION,
    CS4,
    UNARY_MINUS,
    CS5,
    NOT,
    CS6,
    MULTIPLICATION_IMPLICIT,
    MULTIPLICATION,
    CS7,
    ADDITION,
    CS8,
    BITWISE0,
    CS9,
    SPACESHIP,
    CS10,
    COMPARISONS1,
    CS11,
    COMPARISONS2,
    CS12,
    BITWISE1,
    CS13,
    BITWISE2,
    CS14,
    BITWISE3,
    CS15,
    LOGIC1,
    CS16,
    LOGIC2,
    CS17,
    LOGIC3,
    CS18,
};

enum operatorAttributes : char8_t
{
    NOP=0,
    BINARY=1<<0,
    U_PREFIX=1<<1,
    U_POSTFIX=1<<2,
    L_ASSOCIATIVE=1<<3,
    R_ASSOCIATIVE=1<<4,
    FUNCTION_LIKE=U_PREFIX|R_ASSOCIATIVE,
};

inline operatorAttributes operator & (operatorAttributes a, operatorAttributes b) {return static_cast<operatorAttributes>(std::to_underlying(a)&std::to_underlying(b));}
inline operatorAttributes operator | (operatorAttributes a, operatorAttributes b) {return static_cast<operatorAttributes>(std::to_underlying(a)|std::to_underlying(b));}
inline operatorAttributes operator + (operatorAttributes a, operatorAttributes b) {return static_cast<operatorAttributes>(std::to_underlying(a)+std::to_underlying(b));}
enum class tokenType : char8_t
{
    BINARYOP,
    UNARYOP,
    VARIABLE,
    CONSTANT,
    FUNCTION, // Simple baby functions
    
    SUBEXPR,
    // Multiarg functions
    IF,
    NUMBER,
    ROOT,
    LOG,
    DIFF,
    MEAN,
    MEDIAN,
    STDEVP,
    GCF,
    LCM,
    RNDINT,
    ROUND,
    TRUNC,
    RNDSEL,
    ABS,
    MAX,
    SMAX,
    SMIN,
    SABS,
    MIX,
    MIN,
    ATAN2,
    FROM_HEX,
    FROM_BIN,
    FROM_OCT,
    IN_RANGE,

    CUSTOMFN,
    CUSTOMOP,

    INVALID, // Placeholder
};

enum class tokenCategory : char8_t
{
    NUMBER,
    FUNCTION,
    SUBEXPR,
    OPERATOR,
    INVALID
};

bool isNumberPart(char input);

inline bool isRealNumber(const std::string &input, bool disallowSpecials=false);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Frac
{
    float numer{};
    float denom{};
    Frac(float inX, float inY) : numer(inX), denom(inY){}
};

struct Options
{
    bool graph{};
    cpp_complex<maxPrecision> xMin{};
    cpp_complex<maxPrecision> xMax{};  
    cpp_complex<maxPrecision> xStep{}; // Hey, reference
    cpp_complex<maxPrecision> aroundTruthinessLeniency{0.01};
    bool interpolateDiscontinuities{};
    bool prioritizeImplicitMultiplication{true};
    // Used to prevent defining a recursive function from the graphing menu. 
    // This is still possible in scripts (and you can trap the calculator in a loop until you hit the recursion limit)
    std::string definesFunction="";
    bool prettyPrinting{true};
    std::string ans;
    
};



struct Function
{
    bool operator ==(const Function &other) const
    {
        return this->definition==other.definition && this->argNames==other.argNames && this->argc == other.argc;
    }

    bool operator !=(const Function &other) const
    {
        return !(this->definition==other.definition && this->argNames==other.argNames && this->argc == other.argc);
    }
    std::string definition;
    std::vector<std::string> argNames;
    size_t argc{1};
};

struct Operation
{
    std::string m_def;
    pass m_precedence;
    operatorAttributes m_info;

    std::array<std::string, 2> m_argNames{};

    bool operator ==(const Operation &other)
    {
        return m_precedence==other.m_precedence && m_info==other.m_info && m_argNames==other.m_argNames;
    }
    bool operator !=(const Operation &other)
    {
        return !(m_precedence==other.m_precedence && m_info==other.m_info && m_argNames==other.m_argNames);
    }
};

struct CustomSymbols
{
    std::unordered_map<std::string, std::string> m_variables;
    std::unordered_map<std::string, lessetB::Function> m_functions;
    std::unordered_map<std::string, lessetB::Operation> m_operations;
};

bool evaluateEquation(Options &options, bool passedInAsArg, bool passedCalculationsFile, std::string &equation, std::string &resultHistory,
                      std::string &result, std::unordered_map<std::string, std::string> &userVariables,
                      std::unordered_map<std::string, Function> &userFunctions, std::unordered_map<std::string, Operation> &userOperations);



class Token;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "globals.hpp"


[[nodiscard]] inline std::string cleanInput(const std::string& s)
{
    std::string equation=s;
    for(size_t i{}; i < equation.length(); i++)
        if(equation.at(i) < 32) equation.erase(i--, 1); // Delete unprintable characters and unicode
    for(size_t i{}; i < equation.length(); i++)
    {
        if(globals::useDecimalComma && equation.at(i) == ',') equation.at(i) = '.';
        if(globals::useDecimalComma && equation.at(i) == ';') equation.at(i) = ',';
        if(!(isValidInput(equation.at(i)))) equation.erase(equation.begin() + i--); // Basic garbage removal
    }
    return equation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Token
{

    private:
    
    std::string m_value{};
    graphNum m_num{NAN,NAN};
    tokenType m_type{};
    operatorAttributes m_info{NOP};
    tokenCategory m_category{};
    pass m_precedence{INITIAL};
    
    

    ///////////////////////////////////////////////
    [[nodiscard]] tokenType determineType(std::string &value)
    {

        if(isRealNumber(value)) return tokenType::NUMBER;

        for(size_t i{MAX_KEYWORD_LENGTH}; i>0; i--)
        {
            if(i>value.length())
            {
                i=value.length();
                if(i==0) return tokenType::INVALID;
            }

            if(globals::userFunctions.find(value.substr(0,i))!=globals::userFunctions.end())
            {
                return tokenType::CUSTOMFN;
            }
        }

        for(size_t i{MAX_KEYWORD_LENGTH}; i>0; i--)
        {
            if(i>value.length())
            {
                i=value.length();
                if(i==0) return tokenType::INVALID;
            }
            if(globals::userOperations.find(value.substr(0,i))!=globals::userOperations.end())
            {
                return tokenType::CUSTOMOP;
            }
        }

        if(value.length()>0 && value.at(0)=='(')
        {
            std::stringstream ss;
            ss<<value;
            bool isComplexNum{true};
            std::complex<float> num(NAN,NAN);
            ss<<')';
            ss>>num;

            if(num.real()!=num.real() && num.imag()!=num.imag())
            {
                isComplexNum=false;
            }
            if(isComplexNum) return tokenType::CONSTANT;
        }
        
        if((globals::userVariables.find(value)!=globals::userVariables.end()) || globals::constants.find(value)!=globals::constants.end()) return tokenType::CONSTANT;

        if(value=="h*") return tokenType::BINARYOP;

        else if(isSubexpr(value)) return tokenType::SUBEXPR;
        else if(isAbs(value)) return tokenType::ABS;
        tokenType m_typeCandidate=isMultiArgFunction(value);

        if(m_typeCandidate==tokenType::INVALID)
        {
            std::string candidate;
            for(size_t i{MAX_KEYWORD_LENGTH}; i>0; i--)
            {
                if(i>value.length())
                {
                    i=value.length();
                    if(i==0) return tokenType::INVALID;
                }
                candidate=value.substr(0,i);
                if(globals::symbols.find(candidate)!=globals::symbols.end())
                {
                    return globals::symbols.find(candidate)->second;
                }
            }
        }
        return m_typeCandidate;
    }

    ///////////////////////////////////////////////
    tokenType isMultiArgFunction(const std::string &input)
    {
        size_t offset{};
        tokenType type{tokenType::INVALID};

        for(size_t i{MAX_KEYWORD_LENGTH}; i>0; i--)
        {
            if(globals::multiArgFunctions.find(input.substr(0,i))!=globals::multiArgFunctions.end())
            {
                type=globals::multiArgFunctions.find(input.substr(0,i))->second;
                offset=globals::multiArgFunctions.find(input.substr(0,i))->first.length()+1;
                break;
            }
        }
        if(type==tokenType::INVALID) return type;
        for(size_t i{offset}; i<input.length(); i++)
        {
            m_value.push_back(input.at(i));
        }
        if(offset>=input.length()) return tokenType::INVALID;
        return type;
    }
    ///////////////////////////////////////////////
    bool isAbs(std::string &input)
    {
        if((input.at(0)!='|') && input.find("abs(")!=0) return false;
        
        if(input.at(0)=='|') for(size_t i{1}; i<input.length()-1; i++) m_value.push_back(input.at(i));
        else for(size_t i{4}; i<input.length(); i++) m_value.push_back(input.at(i));
        if(input.at(input.length()-1)!='|') m_value.push_back(input.at(input.length()-1));
        for(size_t i{}; i<input.length(); i++)
        {
            if(input.at(i)!='|') return true;
        }
        return false;
    }    
    ///////////////////////////////////////////////
    [[nodiscard]] static bool isSubexpr(std::string &input)
    {
        if(input.find(')')!=std::string::npos && input.length()<2) return false;
        if(input.at(0)=='(')
        {
            input.erase(0, 1);
            return true;
        }
        return false;
    }
    ///////////////////////////////////////////////
    [[nodiscard]] static std::string replaceConstants(const std::string &input)
    {

        if(globals::constants.find(input)!=globals::constants.end()) return globals::constants.find(input)->second;
        if(globals::userVariables.find(input)!=globals::userVariables.end()) return globals::userVariables.find(input)->second;

        return input;
    }
    ///////////////////////////////////////////////
    [[nodiscard]]operatorAttributes determineOperatorAttributes()
    {
        if(m_category==tokenCategory::OPERATOR)
        {
            switch(m_type)
            {
                case tokenType::BINARYOP: 
                {
                    if(m_value=="^" || m_value=="**") return BINARY+R_ASSOCIATIVE;
                    return BINARY+L_ASSOCIATIVE;
                }
                case tokenType::UNARYOP:
                {
                    if(m_value=="-" || m_value=="NOT" || m_value=="B!" || m_value=="~") return U_PREFIX+L_ASSOCIATIVE;
                    
                    else return U_POSTFIX+L_ASSOCIATIVE;
                }
                default: return NOP;
            }
        }
        else if(m_category==tokenCategory::FUNCTION)
        {
            return U_PREFIX+R_ASSOCIATIVE;
        }
        else return NOP;
    }
    ///////////////////////////////////////////////
    [[nodiscard]] static tokenCategory determineTokenCategory(tokenType type) 
    {
        if(type==tokenType::NUMBER || type==tokenType::VARIABLE || type==tokenType::CONSTANT) return tokenCategory::NUMBER;

        if(type==tokenType::FUNCTION)                                                                              return tokenCategory::FUNCTION;

        if(type==tokenType::BINARYOP || type==tokenType::UNARYOP || type==tokenType::CUSTOMOP)                     return tokenCategory::OPERATOR;

        if(type==tokenType::INVALID)                                                                               return tokenCategory::INVALID;
        
        return tokenCategory::SUBEXPR;
    }
    ///////////////////////////////////////////////
    ///////////////////////////////////////////////

    public:

    ///////////////////////////////////////////////
    [[nodiscard]] pass precedence()
    {
        if(m_precedence!=INITIAL) return m_precedence;
        if(m_category==tokenCategory::OPERATOR)
        {
            if(!globals::options.prioritizeImplicitMultiplication && m_value=="h*") return MULTIPLICATION;
            return globals::opToPriority.at(m_value);
        }
        switch(m_category)
        {
            case tokenCategory::SUBEXPR: return SUBEXPRESSIONS;
            case tokenCategory::FUNCTION: return FUNCTIONS;
            default: return INVALID;
        } 
    }

    [[nodiscard]] operatorAttributes opAttribs()
    {
        return m_info;
    }

    Token(std::string value)
    {
        
        if(globals::options.graph) // The things you do to make graphing faster...
        {
            if(value=="(0,1")
            {
                m_num=graphNum(0,1);
                m_type=tokenType::CONSTANT;
                m_category=tokenCategory::NUMBER;
                m_precedence=INVALID;
                m_info=operatorAttributes::NOP;
                return;
            }
            bool isComplexNum{true};
            tokenType potentialNumberType{tokenType::INVALID};
            if(isRealNumber(value,true))
            {
                
                m_type=tokenType::NUMBER;
                m_category=tokenCategory::NUMBER;
                m_precedence=INVALID;
                m_info=operatorAttributes::NOP;
                
                m_num=sscanfWrapper("("+m_value+",0");
                return;
            }
            int successfullyConverted{};
            m_num=sscanfWrapper(value, successfullyConverted);
            if(successfullyConverted!=2) isComplexNum=false;
            else
            {
                m_type=tokenType::CONSTANT;
                m_category=tokenCategory::NUMBER;
                m_precedence=INVALID;
                m_info=operatorAttributes::NOP;
                return;
            }

            if(globals::userVariables.find(value)!=globals::userVariables.end() || globals::constants.find(value)!=globals::constants.end())
            {
                potentialNumberType=tokenType::CONSTANT;
                m_value=replaceConstants(value);
                if(m_value=="rnd" || m_value=="rndint") potentialNumberType=tokenType::INVALID;
            }
            if(potentialNumberType!=tokenType::INVALID && potentialNumberType!=tokenType::CONSTANT) 
            {
                if(isComplexNum) potentialNumberType=tokenType::CONSTANT;
                
                if(value.find(',')!=std::string::npos) m_num=sscanfWrapper(value);
                else if(value.find('(')==std::string::npos) m_num=sscanfWrapper("("+value+",0");
                else
                {
                    m_num=sscanfWrapper(value);
                }

                if(m_num==m_num || m_value=="nan")
                {
                    m_type=potentialNumberType;
                    m_category=tokenCategory::NUMBER;
                    return;
                }
                
            }
            else if(potentialNumberType==tokenType::CONSTANT)
            {
                if(m_value.find(',')!=std::string::npos) m_num=sscanfWrapper(m_value);
                else m_num=sscanfWrapper("("+m_value+",0");
                m_type=tokenType::CONSTANT;
                m_category=tokenCategory::NUMBER;
                return;
            }
        }

        m_type=determineType(value);
        if(m_type==tokenType::CONSTANT) m_value=replaceConstants(value);
        
        m_category=determineTokenCategory(m_type);
        m_precedence=precedence();
        m_info=determineOperatorAttributes();
        if(m_value.empty())m_value=value;

    }


    Token(const std::pair<std::string,std::string> var)
    {
        m_type=tokenType::CONSTANT;
        m_category=tokenCategory::NUMBER;
        bool skip{};
        if(var.second.find('(')!=std::string::npos)
        {
            if(var.second.find(')')==std::string::npos)
            {
                skip=true;
                m_value=var.second;
                m_num=sscanfWrapper(m_value+",0");
            }
            m_value=var.second;
        }
        else m_value="("+var.second+",0";
        if(!skip) 
        {
            m_num=sscanfWrapper(m_value);
        }
        m_precedence=precedence();
        m_info=determineOperatorAttributes();
    }

    Token(const std::string &value, const Operation &op)
    {
        m_value=value;
        m_type=tokenType::CUSTOMOP;
        m_category=tokenCategory::OPERATOR;
        m_info=op.m_info;
        m_precedence=op.m_precedence;
    }

    Token(const std::string &value, tokenType type)
    {
        if(type==tokenType::INVALID)
        {
            m_value=value;
            m_type=type;
            m_category=tokenCategory::SUBEXPR;
            m_precedence=precedence();
            m_info=determineOperatorAttributes();
            return;
        }
        if(type!=tokenType::CUSTOMFN) isMultiArgFunction(value);
        if(type==tokenType::CONSTANT)
        {
            m_value=replaceConstants(value);
            if(globals::nonNumericalConstants.find(m_value)==globals::nonNumericalConstants.end())
            {
                m_num=sscanfWrapper(m_value);
            }
        }
        else if(type==tokenType::NUMBER && globals::options.graph) 
        {
            m_num=sscanfWrapper("("+value+",0");
            m_value=value;
        }
        if(type==tokenType::ABS)
        {
            std::string tmp=value;
            m_value.clear();
            isAbs(tmp);
        }
        if(m_value.empty()) m_value=value;
        m_type=type;
        m_category=determineTokenCategory(type);
        m_precedence=precedence();
        m_info=determineOperatorAttributes();
    }

    Token(tokenType type, const std::string value)
    {
        if(type==tokenType::INVALID)
        {
            m_value=value;
            m_type=type;
            m_category=tokenCategory::SUBEXPR;
            m_precedence=precedence();
            m_info=determineOperatorAttributes();
            return;
        }
        if(isMultiArgFunction(value)!=tokenType::INVALID);
        else if(type==tokenType::CONSTANT)
        {
            m_value=replaceConstants(value);
            if(globals::nonNumericalConstants.find(value)==globals::nonNumericalConstants.end())
            m_num=sscanfWrapper(m_value);
        }
        else if(type==tokenType::NUMBER) 
        {
            m_num=sscanfWrapper("("+value+",0");
            m_value=value;
        }
        else m_value=value;
        if(type==tokenType::ABS)
        {
            std::string tmp=value;
            m_value.clear();
            isAbs(tmp);
        }
        m_type=type;
        m_category=determineTokenCategory(type);
        m_precedence=precedence();
        m_info=determineOperatorAttributes();
    }
    
    Token(cpp_complex<maxPrecision> &value)
    {
        m_type=tokenType::NUMBER;        
        m_category=tokenCategory::NUMBER;
        m_value=value.str(maxPrecision);
        m_precedence=precedence();
        m_info=determineOperatorAttributes();
    }


    Token(graphNum value)
    {
        m_type=tokenType::NUMBER;        
        m_category=tokenCategory::NUMBER;
        m_num=value;
        m_precedence=precedence();
        m_info=determineOperatorAttributes();
    }
    ///////////////////////////////////////////////
    template<typename T>
    [[nodiscard]] T number(T xValue=NAN) const
    {
        if constexpr((std::is_same_v<T,cpp_complex<maxPrecision>> || std::is_same_v<T,graphNum>))
        {
            if(xValue.real()!=NAN && this->m_type==tokenType::VARIABLE)
            {
                return xValue;
            }

            if(m_value=="rnd" || m_value=="rndint") return NAN;

            if(m_type!=tokenType::NUMBER && m_type!=tokenType::CONSTANT) return NAN;
            if constexpr(std::is_same<T,cpp_complex<maxPrecision>>()) return static_cast<cpp_complex<maxPrecision>>(m_value);
            else if constexpr(std::is_same<T,graphNum>()) return m_num;
        }
        else return NAN;
    }
    ///////////////////////////////////////////////
    std::string value() const
    {
        if(!m_value.empty()) return m_value;
        else
        {
            std::ostringstream oss;
            oss.precision(std::numeric_limits<graphNumType>::digits10);
            oss<<m_num;
            return oss.str();
        }
    }  
    tokenType type() const
    {
        return m_type;
    }
    tokenCategory category() const
    {
        return m_category;
    }
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<Token> getTokens(const std::string&, bool resetFirstRun=false, bool noMemoize=false, const std::unordered_map<std::string, std::string> &fnCallParams=std::unordered_map<std::string, std::string>());
void parseMultiArgFunction(const std::string &input, std::vector<Token> &tokens, std::string functionName, size_t &i, bool &inFunctionCall, size_t argCount=1);
inline Operation getOperationFromArgs(std::string &sig, const std::string &def, pass precedence, operatorAttributes info, std::unordered_map<std::string, Operation> &operationContainer);

template<typename T=cpp_complex<maxPrecision>>
T calculation(std::vector<Token>, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams=std::unordered_map<std::string, std::string>());

std::pair<std::vector<graphNumType>,std::vector<graphNumType>> calculationCallerGraphing(std::vector<Token> &tokens, graphNumType xValue, graphNumType xValueMax);
inline std::string calculationCallerTable(std::vector<Token> &tokens, cpp_complex<maxPrecision> xValue, cpp_complex<maxPrecision> xValueMax, size_t totalCalculations);

template<typename T=cpp_complex<maxPrecision>>
T evaluateUnary(Token&, Token&, const T &xValue);

template<typename T=cpp_complex<maxPrecision>>
T evaluateBinary(Token&, Token&, Token&, const T &xValue);

template<typename T=cpp_complex<maxPrecision>>
bool evaluateArgs( const Token &arg, const T &xValue, std::vector<T>&argVals, const std::unordered_map<std::string, std::string> &fnCallParams, size_t argsToEval=SIZE_MAX);

Frac decimalToFraction(cpp_complex<maxPrecision> enumerator, size_t precision=15);

template <typename T>
T evaluateFromHex(const Token &arg);
template <typename T>
T evaluateFromBin( const Token &arg);
template <typename T>
T evaluateFromOct( const Token &arg);

template <typename T=cpp_complex<maxPrecision>> T evaluateAbs(const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams);
template <typename T=cpp_complex<maxPrecision>> T evaluateInrange(const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams);
template <typename T=cpp_complex<maxPrecision>> T evaluateIf(const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams);
template <typename T=cpp_complex<maxPrecision>> T evaluateLog(const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams);
template <typename T=cpp_complex<maxPrecision>> T evaluateRoot(const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams);
template <typename T=cpp_complex<maxPrecision>> T evaluateMean(const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams);
template <typename T=cpp_complex<maxPrecision>> T evaluateMedian(const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams);
template <typename T=cpp_complex<maxPrecision>> T evaluateStdevp(const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams);
template <typename T=cpp_complex<maxPrecision>> T evaluateRndsel(const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams);
template <typename T=cpp_complex<maxPrecision>> T evaluateMax(const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams);
template <typename T=cpp_complex<maxPrecision>> T evaluateLeast(const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams);
template <typename T=cpp_complex<maxPrecision>> T evaluateGcf(const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams);
template <typename T=cpp_complex<maxPrecision>> T evaluateLcm(const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams);
template <typename T=cpp_complex<maxPrecision>> T evaluateRound( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams);
template <typename T=cpp_complex<maxPrecision>> T evaluateFloor( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams);
template <typename T=cpp_complex<maxPrecision>> T evaluateAtan2( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams);
template <typename T=cpp_complex<maxPrecision>> T evaluateCustomFn( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams);
template<typename T> inline T sstepCmpl(T x);
template<typename T> inline T sstep(T x);
template<typename T> inline T normalize(T val, T max, T min);
bool isEmptyEquation(std::string equation);
void addSeparatorAroundDigitGroups(std::string &num, char decimalSymbol, const std::string &separator);
std::string formatNumber(std::string result, const Options &options, const bool noFractions=true, const bool noConstants=false);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

[[nodiscard]] inline std::string replaceUnicode(std::string equation) 
{
    for(size_t i{}; i<equation.length(); i++)
    {
        if(equation.find("≤", i) == i) equation.replace(i, sizeof("≤") - 1, "<=");
        else if(equation.find("ᵉ", i) == i) equation.replace(i, sizeof("ᵉ") - 1, "ec");
        else if(equation.find("α", i) == i) equation.replace(i, sizeof("α") - 1, "a");

        else if(equation.find("τ", i) == i) equation.replace(i, sizeof("τ") - 1, "tau");
        else if(equation.find("ξ", i) == i) equation.replace(i, sizeof("ξ") - 1, "rnd");
        else if(equation.find("∞", i) == i) equation.replace(i, sizeof("∞") - 1, "inf");
        else if(equation.find("φ", i) == i) equation.replace(i, sizeof("φ") - 1, "phi");
        else if(equation.find("√", i) == i) equation.replace(i, sizeof("√") - 1, "sqrt");
        else if(equation.find("∛", i) == i) equation.replace(i, sizeof("∛") - 1, "cbrt");
        else if(equation.find("∜", i) == i) equation.replace(i, sizeof("∜") - 1, "qtrt");
        else if(equation.find("⊕", i) == i) equation.replace(i, sizeof("⊕") - 1, "XOR");
        else if(equation.find("¬", i) == i) equation.replace(i, sizeof("¬") - 1, "NOT");
        else if(equation.find("∨", i) == i) equation.replace(i, sizeof("∨") - 1, "OR");
        else if(equation.find("∧", i) == i) equation.replace(i, sizeof("∧") - 1, "AND");
        else if(equation.find("≥", i) == i) equation.replace(i, sizeof("≥") - 1, ">=");
        else if(equation.find("−", i) == i) equation.replace(i, sizeof("−") - 1, "-");
        else if(equation.find("≠", i) == i) equation.replace(i, sizeof("≠") - 1, "=!");
        else if(equation.find("÷", i) == i) equation.replace(i, sizeof("÷") - 1, "/");
        else if(equation.find("×", i) == i) equation.replace(i, sizeof("×") - 1, "*");
        else if(equation.find("π", i) == i) equation.replace(i, sizeof("π") - 1, "pi");
        else if(equation.find("γ", i) == i) equation.replace(i, sizeof("γ") - 1, "eul");
        else if(equation.find("ℯ", i) == i) equation.replace(i, sizeof("ℯ") - 1, "e");
        else if(equation.find("ᵉ", i) == i) equation.replace(i, sizeof("ᵉ") - 1, "ec");
        else if(equation.find("≈", i) == i) equation.replace(i, sizeof("≈") - 1, "AROUND");
        else if(equation.find("H₀", i) == i) equation.replace(i, sizeof("H₀") - 1, "H0");
        else if(equation.find("Z₀", i) == i) equation.replace(i, sizeof("Z₀") - 1, "Z0");
        else if(equation.find("U₀", i) == i) equation.replace(i, sizeof("U₀") - 1, "U0");
        else if(equation.find("mₐ", i) == i) equation.replace(i, sizeof("mₐ") - 1, "ma");
    }
    return equation;
}
inline bool evaluateEquation(Options &options, bool passedInAsArg, bool passedCalculationsFile, std::string &equation, std::string &resultHistory,
                             std::string &result, std::unordered_map<std::string, std::string> &userVariables,
                             std::unordered_map<std::string, Function> &userFunctions,
                             std::unordered_map<std::string, Operation> &userOperations)
{
    #ifdef LESSET_INCLUDE_DEBUG
    // if(globals::debugCout)
    //     std::cout << "evaluateEquation()\n";
    #endif
    globals::oss.precision(maxPrecision);
    std::string initialEquation = equation;
    globals::error = false;
    globals::userFunctions = userFunctions;
    globals::userVariables = userVariables;
    globals::userOperations = userOperations;
    globals::passedInAsArg = passedInAsArg;
    globals::passedCalculationsFile = passedCalculationsFile;
    globals::options = options;
    std::ostringstream resultAsOSStream;
    resultAsOSStream.precision(maxPrecision);
    getTokens("", true);

    if(equation.find('#') != std::string::npos) equation.erase(equation.find('#'));

    if(equation.length() == 0) return globals::error;

    if(!options.graph && equation=="fish." || equation=="fish") // Fish.
    {
        result += "fish.";
        return globals::error;
    }

    equation=cleanInput(replaceUnicode(equation));


    if(equation.length() == 0)
    {
        globals::error = true;
        result += "No valid input\n";
        equation.clear();
        globals::points.first = {};
        globals::points.second = {};
        return globals::error;
    }

    std::vector<Token> tokens = getTokens(equation);

    if(!globals::errorMessage.empty() && !passedCalculationsFile)
    {
        globals::error = true;
        result = globals::errorMessage;
        globals::errorMessage.clear();
        resultAsOSStream.str("");
        resultAsOSStream.clear();
        equation.clear();
        tokens.clear();
        // globals::tokenMemory.clear();

        return globals::error;
    }

    CustomSymbols symbols(userVariables,userFunctions,userOperations);
    bool hasX = containsX(equation, symbols);

    if(tokens.size() == 0 && !options.graph)
    {
        resultAsOSStream.str("");
        resultAsOSStream.clear();
        equation.clear();
        tokens.clear();
        globals::tokenMemory.clear();

        return globals::error;
    }

    if(!hasX && !options.graph) // No x found
    {
        resultAsOSStream << calculation<cpp_complex<maxPrecision>>(tokens, cpp_complex<maxPrecision>(NAN, NAN));

        if(resultAsOSStream.str()=="nan")
        {
            globals::options.ans = "nan";
            resultAsOSStream.str("");
            resultAsOSStream.clear();
            resultAsOSStream << "Not a Number";
        }
        else if(resultAsOSStream.str() == "-0")
        {
            globals::options.ans = '0';
            resultAsOSStream.str("");
            resultAsOSStream.clear();
            resultAsOSStream << "0";
        }
        else globals::options.ans = resultAsOSStream.str();

        for(size_t i{}; i < tokens.size(); i++)
        {
            if(globals::options.prettyPrinting &&
               (tokens.at(i).value() == "<" || tokens.at(i).value() == ">" || tokens.at(i).value() == "=" || tokens.at(i).value() == "=!" ||
                    tokens.at(i).value() == "<=" || tokens.at(i).value() == "OR" || tokens.at(i).value() == "AND" || tokens.at(i).value() == "XOR" ||
                    tokens.at(i).value() == "AROUND" || tokens.at(i).value() == ">=" || tokens.at(i).value() == "<=>" ||
                    tokens.at(i).value() == "NOT" ||  tokens.at(i).type()==tokenType::IN_RANGE))
            {

                if(resultAsOSStream.str() == "1") resultAsOSStream.str("true");
                else if(resultAsOSStream.str() == "0") resultAsOSStream.str("false");
            }
        }

        if(!passedCalculationsFile) result = resultAsOSStream.str();
        else
        {
            result += equation + "=" + resultAsOSStream.str() + '\n';
        }
    }
    else if(!options.graph)
    {
        // result="";
        std::cout.precision(maxPrecision);
        resultAsOSStream.precision(maxPrecision);
        if(options.xStep == 0) options.xStep = INFINITY;
        size_t i{};
        size_t totalCalculations = static_cast<size_t>(abs(options.xMax - options.xMin) / abs(options.xStep)) + 1;
        if(totalCalculations > 100'000)
        {
            options.xMax = options.xMin + options.xStep * 100'000;
            totalCalculations = 100'000;
        }

        for(cpp_complex<maxPrecision> xValue{options.xMin, 0}; xValue.real() <= cpp_complex<maxPrecision>{options.xMax + 0.000'000'001}.real();
            xValue += options.xStep)
        {
            i++;
            std::ostringstream xValueAsOSStream;
            if(xValue.real() > (-0.000'02) && xValue.real() < 0.000'02 && options.xStep.real() > 0.000'02) xValue = 0;
            xValueAsOSStream << xValue;
            xValue = static_cast<cpp_complex<maxPrecision>>(xValueAsOSStream.str());

            resultAsOSStream << calculation<cpp_complex<maxPrecision>>(tokens, xValue);
            bool isNumber{true};
            if(resultAsOSStream.str()=="nan")
            {
                resultAsOSStream.str("Not a Number");
                isNumber = false;
            }
            if(resultAsOSStream.str() == "-0")
            {
                resultAsOSStream.str("");
                resultAsOSStream.clear();
                resultAsOSStream << "0";
            }

            for(size_t i{}; i < tokens.size(); i++)
            {
                if(globals::options.prettyPrinting &&
                   (tokens.at(i).value() == "<" || tokens.at(i).value() == ">" || tokens.at(i).value() == "=" || tokens.at(i).value() == "=!" ||
                    tokens.at(i).value() == "<=" || tokens.at(i).value() == "OR" || tokens.at(i).value() == "AND" || tokens.at(i).value() == "XOR" ||
                    tokens.at(i).value() == "AROUND" || tokens.at(i).value() == ">=" || tokens.at(i).value() == "<=>" ||
                    tokens.at(i).value() == "NOT" ||  tokens.at(i).type()==tokenType::IN_RANGE))
                {
                    if(resultAsOSStream.str() == "1")
                    {
                        resultAsOSStream.str("true");
                        isNumber = false;
                    }
                    else if(resultAsOSStream.str() == "0")
                    {
                        resultAsOSStream.str("false");
                        isNumber = false;
                    };
                }
            }
            if(globals::decimalPrecision != maxPrecision && isNumber)
            {
                std::ostringstream oss;
                oss.precision(maxPrecision);
                oss << evaluateRound<cpp_complex<maxPrecision>>(Token(std::string("round(" + resultAsOSStream.str() + ",(" +
                                                                                  std::to_string(globals::decimalPrecision) + ',' +
                                                                                  std::to_string(globals::decimalPrecision) + ')'),
                                                                      tokenType::ROUND),
                                                                NAN, std::unordered_map<std::string, std::string>());
                resultAsOSStream.str("");
                resultAsOSStream << oss.str();
            }

            if(options.prettyPrinting && isNumber)
            {
                std::string n = resultAsOSStream.str();
                addSeparatorAroundDigitGroups(n, '.', globals::separator);
                resultAsOSStream.str(n);
            }

            if(globals::useDecimalComma && xValueAsOSStream.str().find('.') != std::string::npos && !globals::error)
            {
                std::string xValueStr = xValueAsOSStream.str();
                for(size_t i{}; i < xValueStr.length(); i++)
                    if(xValueStr.at(i) == '.') xValueStr.at(i) = ',';
                xValueAsOSStream.str("");
                xValueAsOSStream << xValueStr;
            }
            if(globals::useDecimalComma && resultAsOSStream.str().find(',') != std::string::npos && !globals::error)
            {
                std::string resultStr = resultAsOSStream.str();
                for(size_t i{}; i < resultStr.length(); i++)
                    if(resultStr.at(i) == ',') resultStr.at(i) = ';';
                resultAsOSStream.str("");
                resultAsOSStream << resultStr;
            }
            if(globals::useDecimalComma && resultAsOSStream.str().find('.') != std::string::npos && !globals::error)
            {
                std::string resultStr = resultAsOSStream.str();
                for(size_t i{}; i < resultStr.length(); i++)
                    if(resultStr.at(i) == '.') resultStr.at(i) = ',';
                resultAsOSStream.str("");
                resultAsOSStream << resultStr;
            }

            result += std::to_string(i);
            for(size_t spaces{}; spaces < std::to_string(totalCalculations).length() - std::to_string(i).length(); spaces++)
                result += "  ";
            result += ": ( " + xValueAsOSStream.str() + " ; " + resultAsOSStream.str() + " )\n";
            resultAsOSStream.str("");
            resultAsOSStream.clear();
        }
    }
    else if(!passedCalculationsFile && options.graph)
    {
        globals::points.first.clear();
        globals::points.second.clear();

        if(hasX)
        {
            size_t threadCount{std::thread::hardware_concurrency()};
            std::vector<std::future<std::pair<std::vector<graphNumType>, std::vector<graphNumType>>>> results;
            results.reserve(threadCount);
            globals::points.first.reserve(static_cast<size_t>(abs(options.xMax - options.xMin).real() / options.xStep));
            // if(globals::options.xStep==0) globals::options.xStep=DBL_EPSILON;

            for(size_t i{}; i < threadCount; i++)
            {
                graphNumType perThreadRange{abs(options.xMax - options.xMin) / threadCount};
                graphNumType thisThreadOffset{perThreadRange * i};
                graphNumType thisThreadXValue{static_cast<graphNumType>(options.xMin + thisThreadOffset)};
                results.push_back(std::async(calculationCallerGraphing, std::ref(tokens), thisThreadXValue, perThreadRange + thisThreadXValue));
            }

            for(size_t i{}; i < threadCount; i++)
            {
                std::pair<std::vector<graphNumType>, std::vector<graphNumType>> thisThreadPoints = results.at(i).get();
                globals::points.first.append_range(thisThreadPoints.first);
                globals::points.second.append_range(thisThreadPoints.second);
            }
        }
        else // Graphs without x are constant and thus only need to be
             // calculated once.
        {
            graphNum value = calculation<graphNum>(tokens, NAN);
            globals::options.xStep *= 6; // Less points
            size_t amount = static_cast<size_t>((globals::options.xMax.real() - globals::options.xMin.real()) / globals::options.xStep.real()) + 3;
            graphNumType xValue = static_cast<graphNumType>(globals::options.xMin.real());
            globals::points.first.reserve(amount);
            globals::points.second.reserve(amount);

            for(size_t i{}; i < amount; i++) // A few extra points.
            {
                globals::points.first.emplace_back(xValue);
                if(value.imag() == 0) globals::points.second.emplace_back(value.real());
                else globals::points.second.emplace_back(NAN);

                xValue += static_cast<graphNumType>(globals::options.xStep.real());
            }
        }
        // if(globals::debugCout)
        // {
        //     std::cout << "Points calculated for graph " << equation << " : (" << globals::points.first.size() << " ; " << globals::points.second.size()
        //               << ")\n";
        //     globals::debugCoutUsed = true;
        // }
    }
    bool isFraction{};
    bool isKnownConstant{};
    if(options.graph) globals::error=false;
    if(!globals::errorMessage.empty() && !passedCalculationsFile)
    {
        globals::error = true;
        result = globals::errorMessage;
        globals::errorMessage.clear();
        goto cleanup;
    }

    // Show results equal to a constant as that constant
    if(!hasX && options.prettyPrinting && globals::valueToConstant.find(resultAsOSStream.str()) != globals::valueToConstant.end())
    {
        if(equation != resultAsOSStream.str())
        {
            if(!passedCalculationsFile) result = globals::valueToConstant.find(resultAsOSStream.str())->second;
            isKnownConstant = true;
        }
    }
    if(!hasX && resultAsOSStream.str().length() > 1 && options.prettyPrinting &&
       globals::valueToConstant.find(resultAsOSStream.str().substr(1)) != globals::valueToConstant.end())
    {
        if(equation != globals::valueToConstant.find(resultAsOSStream.str().substr(1))->second)
        {
            if(!passedCalculationsFile) result = "-" + globals::valueToConstant.find(resultAsOSStream.str().substr(1))->second;
            // else
            // {
            //     result += "-" + equation + "=" + globals::valueToConstant.find(resultAsOSStream.str().substr(1))->second + '\n';
            // }
            isKnownConstant = true;
        }
    }

    // Check for fractions
    if(!hasX && options.prettyPrinting && isRealNumber(resultAsOSStream.str(), true) && !isKnownConstant &&
       resultAsOSStream.str().find('e') == std::string::npos)
    {
        Frac frac = decimalToFraction(static_cast<cpp_complex<maxPrecision>>(resultAsOSStream.str()));

        std::string currentResult = resultAsOSStream.str();
        resultAsOSStream.str("");
        resultAsOSStream << frac.numer << '/' << frac.denom;
        static_cast<cpp_complex<maxPrecision>>(currentResult).real();

        // Just try a couple times with more and more decimal places and see if
        // the funny spits out a reasonable fraction lmao
        for(size_t i{10}; (frac.numer == INFINITY || abs(calculation<cpp_complex<maxPrecision>>(getTokens(resultAsOSStream.str(), false, true), NAN) -
                                                         static_cast<cpp_complex<maxPrecision>>(currentResult))
                                                             .real() >= cpp_complex<maxPrecision>(0.000000000000000000000000000000001).real()) &&
                          i <= 20;
            i++)
        {
            frac = decimalToFraction(static_cast<cpp_complex<maxPrecision>>(currentResult), i);
            resultAsOSStream.str("");
            resultAsOSStream << frac.numer << '/' << frac.denom;
        }

        if(equation != resultAsOSStream.str() &&
           // std::fmod(frac.y,10)!=0 && // Stops something like 3.307->
           // 3307/1000
           frac.denom != 1 && abs(frac.numer) != INFINITY &&
           abs(calculation<cpp_complex<maxPrecision>>(getTokens(resultAsOSStream.str(), false, true), NAN) -
               static_cast<cpp_complex<maxPrecision>>(currentResult).real()) < cpp_complex<maxPrecision>(0.000000000000000000000000000000001).real())
        {
            isFraction = true;
            if(!passedCalculationsFile) result = resultAsOSStream.str();
            else result += equation + "=" + resultAsOSStream.str() + '\n';
        }
    }
    if(globals::decimalPrecision != maxPrecision && !isKnownConstant && !isFraction && !hasX && !options.graph)
    {
        // Total hack. I don't care though.
        std::ostringstream oss;
        oss.precision(maxPrecision);

        oss << evaluateRound<cpp_complex<maxPrecision>>(Token(tokenType::ROUND,
                                                              std::string("round(" + result + ',' + '(' + std::to_string(globals::decimalPrecision) +
                                                                          ',' + std::to_string(globals::decimalPrecision) + "))")),
                                                        NAN, std::unordered_map<std::string, std::string>());
        result = oss.str();
    }
    if(!hasX && !isKnownConstant) options.ans = result;
    if(!hasX && !isKnownConstant && !options.graph && options.prettyPrinting && !isFraction && result.find('e') == std::string::npos)
    {
        addSeparatorAroundDigitGroups(result, '.', globals::separator);
    }
    if(globals::useDecimalComma && !hasX && (result.find('.') != std::string::npos || result.find(',') != std::string::npos) && !globals::error &&
       !passedInAsArg)
    {
        for(size_t i{}; i < result.length(); i++)
            if(result.at(i) == ',') result.at(i) = ';';
        for(size_t i{}; i < result.length(); i++)
            if(result.at(i) == '.') result.at(i) = ',';
    }

    for(size_t i{1}; i < result.length() - 2 && result.length() > 2 && !globals::error && !passedInAsArg; i++)
    {
        if(std::isdigit(result.at(i - 1)) && result.at(i) == 'e' && (result.at(i + 1) == '-' || result.at(i + 1) == '+') &&
           std::isdigit(result.at(i + 2)))
        {
            result.replace(i, (result.at(i + 1) == '-') + 2 * (result.at(i + 1) == '+'), "×10^");
            i += sizeof("×10^") + result.at(i + 1) == '-';
        }
    }

    if(!hasX && !passedCalculationsFile && !options.graph)
    {
        // This caused me immense pain
        std::string addToHistory = '\n' + initialEquation + "=" + result;
        if(resultHistory.find(addToHistory) == std::string::npos) resultHistory += addToHistory;
    }

cleanup:
    globals::fnCallParams.clear();
    globals::prevUserFunctions = globals::userFunctions;
    globals::prevUserOperations = globals::userOperations;
    resultAsOSStream.str("");
    resultAsOSStream.clear();
    equation.clear();
    tokens.clear();
    // globals::tokenMemory.clear();

    globals::activeRecursions = 0;
    return globals::error;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

[[nodiscard]] inline std::pair<std::vector<graphNumType>, std::vector<graphNumType>> calculationCallerGraphing(std::vector<Token> &tokens, graphNumType xValue, graphNumType xValueMax)
{
    // if(threadNumber==std::thread::hardware_concurrency()-1) xValueMax+=static_cast<graphNumType>(globals::options.xStep);
    std::pair<std::vector<graphNumType>, std::vector<graphNumType>> points;
    points.second.reserve((xValueMax - xValue) / static_cast<graphNumType>(globals::options.xStep.real()));
    points.first.reserve((xValueMax - xValue) / static_cast<graphNumType>(globals::options.xStep.real()));

    for(graphNumType previousXValue{NAN}; xValue < xValueMax; xValue += static_cast<graphNumType>(globals::options.xStep.real()))
    {
        if(previousXValue == xValue) return points;
        graphNum result = calculation<graphNum>(tokens, xValue);
        points.first.emplace_back(xValue);
        if(abs(result.imag()) == 0) points.second.emplace_back(result.real());
        else points.second.emplace_back(NAN);
        previousXValue = xValue;
    }
    return points;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

[[nodiscard]] inline bool isValidInput(const char c)
{
    return !(c=='\t' || c=='\n' || c==' ' || c=='\\' || c=='\'') && c>' ';
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline void parseMultiArgFunction(const std::string &input, std::vector<Token> &tokens, std::string functionName, size_t &i, bool &inFunctionCall, size_t argCount)
{
    size_t initialI{i};
    i=0;
    size_t argFound{1};
    std::string currentToken;
    int nestingLevel{};
    size_t functionNameLength{};
    functionNameLength=functionName.length();

    for(; i<input.length(); i++)
    {
        if(currentToken.empty() && (input.find(functionName, i)==i || functionName=="")) 
            for(; i<input.length(); i++)
            {
                if(!inFunctionCall)
                {
                    currentToken.append(functionName);
                    i+=functionNameLength;
                    inFunctionCall=true;
                    if(i==input.length()-1) continue;
                }
                if(i<input.length()-1 && input.at(i)==',' && nestingLevel==1 && input.at(i+1)!=')') argFound++;

                if((i<input.length()-1 && nestingLevel==0 && input.at(i)=='(' && input.at(i+1)==')') || 
                   (i==input.length()-1 && nestingLevel==0 && input.at(i)=='(')) argFound=0; // No arguments
                   
                if(input.at(i)==')') nestingLevel--;
                else if(input.at(i)=='(') nestingLevel++;
                currentToken.push_back(input.at(i));
                if(nestingLevel==0 || i==input.length()-1)
                {
                    if(argCount>argFound && !globals::passedCalculationsFile)
                    {
                        globals::errorMessage+="Too few arguments for function \"";
                        globals::errorMessage+=functionName;
                        globals::errorMessage+="\" (Expected at least "+ std::to_string(argCount)+", found "+std::to_string(argFound)+")\n";
                        globals::error=true;
                        i+=initialI;
                    }
                    else
                    {
                        if(globals::multiArgFunctions.find(functionName)!=globals::multiArgFunctions.end()) 
                            tokens.emplace_back(currentToken,globals::multiArgFunctions.at(functionName));
                        else tokens.emplace_back(currentToken,tokenType::CUSTOMFN);
                        // else std::cout<<"How??\n";
                        i+=initialI;
                    }

                    return;
                }
            }
    }
    // std::cout<<"How??\n";
    i+=initialI;
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Pretty much a lexer.
[[nodiscard]] inline std::vector<Token> getTokens(const std::string &input, bool resetFirstRun, bool noMemoize, const std::unordered_map<std::string, std::string> &customCallParams)
{
    
    for(std::pair<std::string,Function> fn : globals::userFunctions)
    {
        if(globals::prevUserFunctions.find(fn.first)==globals::prevUserFunctions.end())
        {
            noMemoize=true;
            globals::tokenMemory.clear();   
            break;
        }
        else if(fn.second.definition!=globals::userFunctions.at(fn.first).definition) 
        {
            noMemoize=true;
            globals::tokenMemory.clear();   
            break;
        }
    }
    for(std::pair<std::string,Operation> op : globals::userOperations)
    {
        if(globals::prevUserOperations.find(op.first)==globals::prevUserOperations.end())
        {
            noMemoize=true;
            globals::tokenMemory.clear();   
            break;
        }
        else if(op.second.m_def!=globals::userOperations.at(op.first).m_def) 
        {
            noMemoize=true;
            globals::tokenMemory.clear();   
            break;
        }
    }
    if(!globals::options.graph) noMemoize=true;
    if(globals::tokenMemory.find(input)!=globals::tokenMemory.end() && !noMemoize)
    {
        return globals::tokenMemory.find(input)->second;
    }

    static bool firstRun{true};
    if(resetFirstRun)
    {
        firstRun=true;
        return std::vector<Token>();
    }
    
    int nestingLevel{};
    int absNestingLevel{};
    std::vector<Token> tokens{};
    
    std::string currentToken{};
    bool fixOffByOne{};
    bool inFunctionCall{};
    const size_t inputLen=input.length();
    for(size_t i{}; i<inputLen; i++)
    {
        // Parse |x|... or ||x|| if the user hates me... or ||||x||||. whatever.
        if(input.at(i)=='|') for(;i<inputLen; i++)
        {
            if(!inFunctionCall)
            {
                for(;input.at(i)=='|' && i+1<inputLen;i++)
                {
                    absNestingLevel++;
                    currentToken.push_back('|');
                }
                inFunctionCall=true;
            }
            if(input.at(i)==')') nestingLevel--;
            else if(input.at(i)=='(') nestingLevel++;
            if(nestingLevel==0 && input.at(i)=='|') absNestingLevel--;
            if(absNestingLevel<0)
            {
                globals::errorMessage+="Bad absolute value parentheses\n";
                globals::error=true;
                break;
            }
            if((nestingLevel==0 && absNestingLevel==0 && nestingLevel==0 && input.at(i)=='|') || 
               (i==input.length()-1)) 
            {
                currentToken.push_back(input.at(i));
                tokens.emplace_back(currentToken,tokenType::ABS);
                break;
            }
            else if(i==input.length()-1 && input.at(i)!='|') continue;

            currentToken.push_back(input.at(i));
        }

        // Parse MultiArg Functions
        bool contin{};
        for(size_t j{MAX_KEYWORD_LENGTH}; !inFunctionCall; j--)
        {
            
            if(j>inputLen-i) j=inputLen-i;
            if(j<2) break;
            size_t offset{};
            if(globals::multiArgFunctions.find(input.substr(i,j))!=globals::multiArgFunctions.end())
            {
                offset=globals::multiArgFunctions.find(input.substr(i,j))->first.length();

                if(i+offset<inputLen && input.at(i+offset)=='(')
                {
                    parseMultiArgFunction(input.substr(i),tokens,input.substr(i,offset),i,inFunctionCall);
                    contin=true;
                }
                if(i>=inputLen)
                {
                    globals::error=true;
                    return std::vector<Token>();
                }
                
            }
        }
        if(contin) {inFunctionCall=false; continue;}

        // Custom functions
        for(size_t j{MAX_KEYWORD_LENGTH}; !inFunctionCall && j>0; j--)
        {
            if(j>inputLen-i) j=inputLen-i;
            size_t offset{};
            if(globals::userFunctions.find(input.substr(i,j))!=globals::userFunctions.end() && input.substr(i,j)!=globals::options.definesFunction)
            {
                offset=globals::userFunctions.find(input.substr(i,j))->first.length();

                if(i+offset<inputLen && input.at(i+offset)=='(')
                {
                    parseMultiArgFunction(input, tokens, input.substr(i,j), i, inFunctionCall,globals::userFunctions.at(input.substr(i,j)).argc);
                    contin=true;
                }
                currentToken.clear();
                break;
            }

        }
        if(contin) {inFunctionCall=false; continue;}

        // Parse Subexpression (or complex number written as (real,imag) )
        if(currentToken.empty() && input.at(i)=='(') 
            for(; i<input.length(); i++)
            {
                if(input.at(i)==')') nestingLevel--;
                else if(input.at(i)=='(') nestingLevel++;
                if(nestingLevel!=0)currentToken.push_back(input.at(i));
                if(nestingLevel==0 || i==inputLen-1) break;                
            }

        if(customCallParams!=std::unordered_map<std::string, std::string>())
        {
            std::string candidate;
            int j{MAX_KEYWORD_LENGTH};
            if(j>inputLen-i) j=inputLen-i;
            for(; j>0; j--) // Check short substrs ahead of where you are in equation in descending size and match against function argument names
            {
                candidate=input.substr(i,j);
                
                if(customCallParams.find(candidate)!=customCallParams.end())
                {
                    currentToken=" ";
                    inFunctionCall=true;
                    tokens.emplace_back(std::pair<std::string,std::string>(candidate,customCallParams.at(candidate)));
                    i+=j-1;
                    break;
                }
            }
        }
        
        // Parse other symbols
        if(currentToken.empty())
        {
            std::string candidate;
            size_t j{MAX_KEYWORD_LENGTH};
            if(j>inputLen-i) j=inputLen-i;
            for(; j>0; j--) // Check short substrs ahead of where you are in equation in descending size and match against known symbols
            {
                candidate=input.substr(i,j);
                if(globals::symbols.find(candidate)!=globals::symbols.end() || 
                   globals::userVariables.find(candidate)!=globals::userVariables.end() ||
                   globals::userOperations.find(candidate)!=globals::userOperations.end())
                {
                    currentToken=candidate;
                    if(globals::symbols.find(candidate)!=globals::symbols.end())
                    {
                        tokens.emplace_back(currentToken,globals::symbols.at(candidate));
                    }
                    else if(globals::userOperations.find(candidate)!=globals::userOperations.end())
                    {
                        tokens.emplace_back(currentToken,globals::userOperations.at(candidate));
                    }
                    else tokens.emplace_back(currentToken,tokenType::CONSTANT);
                    i+=j-1;
                    break;
                }
            }
            if(!currentToken.empty())
            {
                currentToken.clear();
                continue;
            }
        }

        // Parse Number
        if(currentToken.empty())
        {
            bool seenE{};
            bool seenDecimalPoint{};
            for(; i<inputLen && std::isdigit(input[i]) || // 5
                
                ( i+1<inputLen && input[i]=='.' && !seenDecimalPoint && std::isdigit(input[i+1]) ) || // .5

                (inputLen>2 && i>0 && std::isdigit(input[i-1]) && input[i]=='e' && !seenE && 
                ((i<inputLen-2 && ( (input[i+1]=='+' || input[i+1]=='-') && std::isdigit(input[i+2]) )) || // 5e±5
                 (i<inputLen-1 && std::isdigit(input[i+1])))) // 5e5

                ; i++)

            // I am deeply sorry.
            {
                
                fixOffByOne=true;
                if(input[i]=='.') seenDecimalPoint=true;
                if(i+1<inputLen &&
                (input.at(i)=='e' &&
                (input.at(i+1)=='+' || input.at(i+1)=='-' || std::isdigit(input.at(i+1)))))
                {
                    seenE=true;
                    currentToken.push_back(input.at(i));
                    i++;
                }
                currentToken.push_back(input.at(i));
            }
            if(fixOffByOne)
            {
                tokens.emplace_back(currentToken,tokenType::NUMBER);
                currentToken.clear();
                fixOffByOne=false;
                i--;
                continue;
            }
        }

        if(inFunctionCall) currentToken.clear();
        if(!currentToken.empty()) tokens.emplace_back(currentToken);
        currentToken.clear();
        inFunctionCall=false;
    }
    if(!currentToken.empty()) tokens.emplace_back(currentToken);
    if(firstRun) firstRun=false;

    
    if(tokens.size()==0) return tokens;
    // Remove some stray operators
    for(size_t i{}; i<tokens.size(); i++)
    {
        if(tokens.at(0).category()==tokenCategory::OPERATOR && !(tokens.at(0).opAttribs()&U_PREFIX)) tokens.erase(tokens.begin());
        else break;
    }

    // Implicit parentheses around single argument functions
    {
        std::string newSubexpr="(";
        bool disallowMinus{};
        for(size_t i{1}; i<tokens.size(); i++)
        {
            if(
                (tokens.at(i-1).type()==tokenType::FUNCTION || 
                 (tokens.at(i-1).type()==tokenType::CUSTOMOP && tokens.at(i-1).opAttribs()==FUNCTION_LIKE && tokens.at(i-1).precedence()==FUNCTIONS)
                ) &&
                (
                 (tokens.at(i).category()==tokenCategory::NUMBER || (tokens.at(i).category()==tokenCategory::OPERATOR && tokens.at(i).precedence()<UNARY_MINUS)) || 
                 (tokens.at(i).value()=="-" && !disallowMinus && i+1<tokens.size() && tokens.at(i+1).category()==tokenCategory::NUMBER) ||
                 ((tokens.at(i).value()=="**" || tokens.at(i).value()=="^") && i+1<tokens.size() && tokens.at(i+1).category()==tokenCategory::NUMBER)
                )
                && (i+1==tokens.size() || tokens.at(i).category()!=tokenCategory::SUBEXPR)
              )
            {
                disallowMinus=true;
                newSubexpr.append(tokens.at(i).value());
                tokens.erase(tokens.begin()+i);
                i--;
                
            }
            else if(newSubexpr.length()>1)
            {
                tokens.insert(tokens.begin()+i, Token(newSubexpr,tokenType::SUBEXPR));
                newSubexpr="(";
                disallowMinus=false;
            }
            
        }
        if(newSubexpr.length()>1)
        {
            tokens.insert(tokens.end(), Token(newSubexpr,tokenType::SUBEXPR));
        }
    }

    // Implicit multiplication, removing unary plus, so on
    for(size_t i{1}; i<tokens.size(); i++)
    {
        using enum tokenCategory;
        const Token implicitMultiply(tokenType::BINARYOP, "h*");

        // Example: 4!!3-> 4!! h* 3
        if(tokens.at(i).category() == NUMBER &&
           (tokens.at(i - 1).category() == OPERATOR && (tokens.at(i - 1).opAttribs() & U_POSTFIX) && tokens.at(i - 1).precedence() < MULTIPLICATION))
            tokens.emplace(tokens.begin() + i++, implicitMultiply);

        // Example: 3x-> 3 h* x
        if((tokens.at(i).type() == tokenType::VARIABLE || tokens.at(i).type() == tokenType::CONSTANT) && tokens.at(i - 1).category() == NUMBER)
            tokens.emplace(tokens.begin() + i++, implicitMultiply);

        // Example: x3-> x h* 3
        if(tokens.at(i).category() == NUMBER && (tokens.at(i - 1).type() == tokenType::VARIABLE || tokens.at(i - 1).type() == tokenType::CONSTANT))
            tokens.emplace(tokens.begin() + i++, implicitMultiply);

        // Example: 3sin x-> 3 h* sinx
        if(tokens.at(i).type() == tokenType::FUNCTION && tokens.at(i - 1).category() == NUMBER ||
          (tokens.at(i).type() == tokenType::CUSTOMOP && tokens.at(i).opAttribs() & U_PREFIX && tokens.at(i).precedence() < MULTIPLICATION && tokens.at(i - 1).category() == NUMBER))
            tokens.emplace(tokens.begin() + i++, implicitMultiply);


        // Example: 3(expr)-> 3 h* (expr)
        if((tokens.at(i).category() == SUBEXPR || tokens.at(i).type() == tokenType::FUNCTION) && tokens.at(i - 1).category() != OPERATOR &&
           tokens.at(i - 1).type() != tokenType::FUNCTION)
            tokens.emplace(tokens.begin() + i++, implicitMultiply);

        // Example: (expr)3 -> (expr) h* 3
        if(tokens[i - 1].category() == SUBEXPR && tokens[i].category() != OPERATOR && tokens[i].category() != SUBEXPR)
            tokens.emplace(tokens.begin() + i++, implicitMultiply);

        // Example: 3-3 -> 3+-3, (expr)-3 -> (expr)+-3
        // Reason: Binary minus is a lie lol
        if(tokens.at(i).value() == "-" && ((tokens.at(i - 1).category() == NUMBER || tokens.at(i - 1).category() == SUBEXPR) ||
                                             (tokens.at(i - 1).opAttribs() & U_POSTFIX)))
            tokens.emplace(tokens.begin() + i++, Token(tokenType::BINARYOP, "+"));

        // Delete unary plus since it does jack
        if(tokens.at(i).value() == "+" && tokens.at(i - 1).category() != NUMBER && (!(tokens.at(i - 1).opAttribs() & U_POSTFIX)) && tokens.at(i - 1).category() != SUBEXPR)
            tokens.erase(tokens.begin() + i--);
    }

    for(size_t i{1}; i < tokens.size(); i++)
    {
        if(i < 1) continue;
        if(tokens.at(i).opAttribs() & BINARY && tokens.at(i - 1).opAttribs() & BINARY) // Example: 3**/3-> 3**3
        {
            tokens.erase(tokens.begin() + i--);
        }
        if(i > 0 && tokens.at(i).value() == "-" && tokens.at(i - 1).value() == "-")
        {
            tokens.erase(tokens.begin() + i - 1, tokens.begin() + i + 1);
            i -= 2;
        }
    }
    for(int i{static_cast<int>(tokens.size()) - 1}; i >= 0; i--)
    {
        if(tokens.at(i).value() == "-" || tokens.at(i).opAttribs() & BINARY || tokens.at(i).type() == tokenType::FUNCTION)
            tokens.erase(tokens.begin() + i);

        else break;
    }

    if(globals::errorMessage.empty() && !noMemoize && customCallParams.empty()) globals::tokenMemory.emplace(input,tokens);
    return tokens;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
void handlePostfixUnary(std::vector<Token> &tokens, int &i, T xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    T evaluatedUnary=evaluateUnary<T>(tokens.at(i), tokens.at(i+1), xValue);
    tokens.at(i)=Token(evaluatedUnary);
    tokens.erase(tokens.begin()+i+1);
    i--;
}

template<typename T>
void handlePrefixUnary(std::vector<Token> &tokens, int &i, T xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    T evaluatedUnary=evaluateUnary<T>(tokens.at(i), tokens.at(i-1), xValue);
    tokens.at(i-1)=Token(evaluatedUnary);
    tokens.erase(tokens.begin()+i);
    i--;
}

template<typename T>
void handleBinary(std::vector<Token> &tokens, int &i, T xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    T evaluatedBinary=evaluateBinary<T>(tokens.at(i-2), tokens.at(i-1), tokens.at(i), xValue);
    tokens.at(i-2)=Token(evaluatedBinary);
    tokens.erase(tokens.begin()+i-1,tokens.begin()+i+1);
    i-=2;
}

template<typename T> [[nodiscard]]
T evaluateCustomBinary(Token &numberTokenLeft, Token &operation, Token &numberTokenRight, const T &xValue)
{
    if(globals::userOperations.find(operation.value())==globals::userOperations.end()) return NAN;

    globals::activeRecursions++;
    if(globals::activeRecursions>700) 
    {
        globals::error=true; 
        globals::errorMessage+="Exceeded recusion limit\n";
        return NAN;
    }

    std::unordered_map<std::string,std::string> params{};

    Operation op = globals::userOperations.at(operation.value());

    std::ostringstream oss;
    if constexpr(std::is_same_v<T, graphNum>) oss.precision(15);
    else oss.precision(maxPrecision);
    oss<<numberTokenLeft.number(xValue);
    params.emplace(op.m_argNames[0],oss.str());
    oss.str("");
    oss<<numberTokenRight.number(xValue);
    params.emplace(op.m_argNames[1],oss.str());
    std::vector<Token> tokens=getTokens(globals::userOperations.find(operation.value())->second.m_def,false,true,params);
    
    T result = calculation<T>(tokens, xValue,params);
    globals::activeRecursions--;
    return result;
}

template<typename T> [[nodiscard]]
T evaluateCustomUnary(Token &number, Token &operation, const T &xValue)
{
    
    if(globals::userOperations.find(operation.value())==globals::userOperations.end()) return NAN;
    globals::activeRecursions++;
    if(globals::activeRecursions>700) 
    {
        globals::error=true; 
        globals::errorMessage+="Exceeded recusion limit\n";
        return NAN;
    }
    std::unordered_map<std::string,std::string> params{};
    static Operation op = globals::userOperations.at(operation.value());

    std::ostringstream oss;
    
    if constexpr(std::is_same_v<T, graphNum>) oss.precision(15);
    else oss.precision(maxPrecision);
    oss<<number.number(xValue);
    params.emplace(op.m_argNames[0],oss.str());
    std::vector<Token> tokens=getTokens(globals::userOperations.find(operation.value())->second.m_def,false,true,params);
    
    T result = calculation<T>(tokens, xValue,params);
    globals::activeRecursions--;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
T calculation(std::vector<Token> tokens, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    if constexpr((std::is_same_v<T,cpp_complex<maxPrecision>> || std::is_same_v<T,graphNum>))
    {
        if(tokens.size()==0) return NAN;
        
        std::ostringstream resultAsOSStream;
        if constexpr(std::is_same_v<T,graphNum>)
            resultAsOSStream.precision(std::numeric_limits<T>::digits10);
        else resultAsOSStream.precision(maxPrecision);
            
        // Replace ans, rnd, rndint with numbers
        for(size_t i{}; i<tokens.size(); i++)
        {
            if(tokens.at(i).type()==tokenType::CONSTANT)
            {
                if(tokens.at(i).value()=="ans")
                {
                    tokens.at(i)=Token(globals::options.ans);
                }
                else if(tokens.at(i).value()=="rnd" || tokens.at(i).value()=="rndint")
                {
                    std::uniform_real_distribution<> graphNumTypeDist(0,1);
                    resultAsOSStream<<graphNumTypeDist(randomMt);
                    std::string randomAsStr {resultAsOSStream.str()};
                    if(tokens.at(i).value()=="rndint" && randomAsStr.find('.')!=std::string::npos) // To get random integers, it literally deletes the decimal point
                    {
                        randomAsStr.erase(randomAsStr.find('.'), 1);
                    }
                    tokens.at(i)=Token(randomAsStr,tokenType::NUMBER);
                    resultAsOSStream.str("");
                    resultAsOSStream.clear();
                }
            }
        }

        resultAsOSStream.str("");
        resultAsOSStream.clear();

        if(tokens.size()==1 && tokens.at(0).category()==tokenCategory::NUMBER) 
        {
            return tokens.at(0).number(xValue);
        }
        if(tokens.size()==1 && tokens.at(0).type()==tokenType::INVALID)
        {
            return NAN;
        }

        size_t pass{SUBEXPRESSIONS};
        for(; pass<=LOGIC1 && !globals::error && (tokens.size()>1 || tokens.at(0).category()==tokenCategory::SUBEXPR); pass++)
        {
            for(int i{}; i<tokens.size() && !globals::error; i++) // Stop trying when error found
            {
                // std::cout<<tokens.at(i).value()<<"  |  Info: "<<static_cast<size_t>(tokens.at(i).opAttribs())<<"  |  Precedende: "<<tokens.at(i).precedence()<<std::endl;

                auto handleOperators = [](std::vector<Token> &tokens, int &i, const T &xValue, size_t precedence, bool rightAssociate, const std::unordered_map<std::string, std::string> &fnCallParams)
                {
                    if (i>1 && 
                        tokens.at(i-1).opAttribs()==BINARY+(rightAssociate ? R_ASSOCIATIVE : L_ASSOCIATIVE) && 
                        precedence==tokens.at(i-1).precedence() && tokens.at(i-2).category()==tokenCategory::NUMBER && tokens.at(i).category()==tokenCategory::NUMBER)
                    {
                        handleBinary(tokens,i,xValue,fnCallParams);
                    }
                    if (i>0 && 
                        tokens.at(i-1).opAttribs()==U_PREFIX+(rightAssociate ? R_ASSOCIATIVE : L_ASSOCIATIVE) &&
                        precedence==tokens.at(i-1).precedence() && tokens.at(i).category()==tokenCategory::NUMBER)
                    {
                        handlePrefixUnary(tokens,i,xValue,fnCallParams);
                        if(rightAssociate) i++;
                    }
                    if (i+1<tokens.size() && 
                        tokens.at(i+1).opAttribs()==U_POSTFIX+(rightAssociate ? R_ASSOCIATIVE : L_ASSOCIATIVE) &&
                        precedence==tokens.at(i+1).precedence() && tokens.at(i).category()==tokenCategory::NUMBER)
                    {
                        handlePostfixUnary(tokens,i,xValue,fnCallParams);
                        if(rightAssociate) i++;
                    }
                    
                };


                if(pass!=SUBEXPRESSIONS) handleOperators(tokens,i,xValue,pass,false,fnCallParams);
                if(i<0) continue;


                if(pass!=SUBEXPRESSIONS && tokens.at(i).opAttribs()&R_ASSOCIATIVE && tokens.at(i).precedence()==pass)
                {
                    for(i=tokens.size()-1; i>=0; i--)
                    {
                        // Account something like x^-1
                        if(i>1 && (tokens.at(i-2).value()=="^" || tokens.at(i-2).value()=="**") && tokens.at(i-1).value()=="-" && tokens.at(i).category()==tokenCategory::NUMBER)
                        {
                            handlePrefixUnary(tokens,i,xValue,fnCallParams);            
                        }
                        handleOperators(tokens,i,xValue,pass,true,fnCallParams);
                    }
                    i=tokens.size()-1;
                    continue;
                }
                if(pass==SUBEXPRESSIONS)
                {
                    if(tokens.at(i).category()==tokenCategory::SUBEXPR)
                    {
                        T evaluatedSubexpr{NAN};
                        globals::activeRecursions++;
                        if(globals::activeRecursions>2000) 
                        {
                            if(!globals::options.graph)
                            {
                                if(globals::activeRecursions<10000)
                                {
                                    globals::error=true; 
                                    globals::errorMessage+="Exceeded recusion limit\n"; 
                                }
                                globals::activeRecursions=10000;
                                return NAN;
                            }
                            else if(globals::activeRecursions>2000*((1.5*(xValue!=xValue))+0.5*globals::threadCount*(xValue==xValue)))
                            {
                                globals::error=true; 
                                globals::activeRecursions--;
                                return NAN;
                            }
                        }
                        
                        switch(tokens.at(i).type())
                        {
                            case tokenType::SUBEXPR: evaluatedSubexpr=calculation<T>(getTokens(tokens.at(i).value(),false,false,fnCallParams), xValue, fnCallParams); break;
                            case tokenType::ABS:     evaluatedSubexpr=evaluateAbs(tokens.at(i), xValue, fnCallParams); break;
                            case tokenType::DIFF:    evaluatedSubexpr=evaluateDiff(tokens.at(i), xValue, fnCallParams); break;
                            case tokenType::MEAN:    evaluatedSubexpr=evaluateMean(tokens.at(i), xValue, fnCallParams); break;
                            case tokenType::MEDIAN:  evaluatedSubexpr=evaluateMedian(tokens.at(i), xValue, fnCallParams); break;
                            case tokenType::STDEVP:  evaluatedSubexpr=evaluateStdevp(tokens.at(i), xValue, fnCallParams); break;
                            case tokenType::MAX:     evaluatedSubexpr=evaluateMax(tokens.at(i), xValue, fnCallParams); break;
                            case tokenType::MIX:     evaluatedSubexpr=evaluateMix(tokens.at(i), xValue, fnCallParams); break;
                            case tokenType::CUSTOMFN:evaluatedSubexpr=evaluateCustomFn(tokens.at(i),xValue, fnCallParams); break;
                            case tokenType::SABS:    evaluatedSubexpr=evaluateSabs(tokens.at(i), xValue, fnCallParams); break;
                            case tokenType::ATAN2:   evaluatedSubexpr=evaluateAtan2(tokens.at(i), xValue, fnCallParams); break;
                            case tokenType::GCF:     evaluatedSubexpr=evaluateGcf(tokens.at(i), xValue, fnCallParams); break;
                            case tokenType::LCM:     evaluatedSubexpr=evaluateLcm(tokens.at(i), xValue, fnCallParams); break;
                            case tokenType::MIN:     evaluatedSubexpr=evaluateMin(tokens.at(i), xValue, fnCallParams); break;
                            case tokenType::RNDSEL:  evaluatedSubexpr=evaluateRndsel(tokens.at(i), xValue, fnCallParams); break;
                            case tokenType::RNDINT:  evaluatedSubexpr=evaluateRndint(tokens.at(i), xValue, fnCallParams); break;
                            case tokenType::ROUND:   evaluatedSubexpr=evaluateRound(tokens.at(i), xValue, fnCallParams); break;
                            case tokenType::TRUNC:   evaluatedSubexpr=evaluateTrunc(tokens.at(i), xValue, fnCallParams); break; 
                            case tokenType::ROOT:    evaluatedSubexpr=evaluateRoot(tokens.at(i), xValue, fnCallParams); break;
                            case tokenType::LOG:     evaluatedSubexpr=evaluateLog(tokens.at(i), xValue, fnCallParams); break;
                            case tokenType::IF:      evaluatedSubexpr=evaluateIf(tokens.at(i), xValue, fnCallParams); break;
                            case tokenType::FROM_HEX:evaluatedSubexpr=evaluateFromHex<T>(tokens.at(i)); break;
                            case tokenType::FROM_BIN:evaluatedSubexpr=evaluateFromBin<T>(tokens.at(i)); break;
                            case tokenType::FROM_OCT:evaluatedSubexpr=evaluateFromOct<T>(tokens.at(i)); break;
                            case tokenType::IN_RANGE:evaluatedSubexpr=evaluateInrange(tokens.at(i), xValue, fnCallParams); break;
                            default:{}                
                        }
                        tokens.at(i)=Token(evaluatedSubexpr);
                        globals::activeRecursions--;
                    }
                }
            }
        }
        // if(globals::activeRecursions>0) globals::activeRecursions--;
        if(tokens.size()==1 && tokens.at(0).number(xValue)==T(-0)) tokens.at(0)=Token(tokenType::NUMBER,"0");
        if(tokens.size()==1 && tokens.at(0).type()==tokenType::VARIABLE) return xValue;
        if(tokens.size()==1 && (tokens.at(0).type()==tokenType::NUMBER|| tokens.at(0).type()==tokenType::CONSTANT)) return tokens.at(0).number(xValue);
        

        // if(globals::errorMessage=="" && !globals::passedCalculationsFile) globals::errorMessage+="Malformed expression\n";
        // globals::error=true;
    }
    return NAN;
}

template<typename T> [[nodiscard]]
T evaluateAbs( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    return abs(calculation<T>(getTokens(arg.value()), xValue,fnCallParams));
}

template <typename T> [[nodiscard]]
bool evaluateArgs( const Token &arg, const T &xValue, std::vector<T>&argVals, const std::unordered_map<std::string, std::string> &fnCallParams, size_t argsToEval)
{
    if(globals::error) return true;
    std::string currentToken;
    int nestingLevel{};
    for(size_t i{}; i<arg.value().length() && nestingLevel>=0 && argVals.size()<argsToEval; i++)
    {
        if(globals::error) return true;
        if(arg.value().at(i)=='(') nestingLevel++;
        else if(arg.value().at(i)==')') nestingLevel--;
        if(nestingLevel<0) break;
        if(!(arg.value().at(i)==',' && nestingLevel==0) && i<arg.value().length()) currentToken.push_back(arg.value().at(i));
        else
        {
            argVals.emplace_back(calculation<T>(getTokens(currentToken,false,false, fnCallParams), xValue, fnCallParams));
            currentToken.clear();
        }
    }
    if(!currentToken.empty()) 
    {
        argVals.emplace_back(calculation<T>(getTokens(currentToken,false,false, fnCallParams), xValue, fnCallParams));
    }
    if(globals::error) return true;
    return false;
}////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline char8_t hexDigitToDecimal(char c)
{
    if(c>='A' && c<='F') return c-'A'+10;
    else if(c>='0' && c<='9') return c-'0';
    else return -1;
}

template <typename T>
T evaluateFromHex( const Token &arg)
{
    std::string asHex = arg.value();
    if(asHex.length()<1) return NAN;
    if(asHex.find("0x")==0) asHex.erase(0,2);
    if(asHex[asHex.length()-1]==')') asHex.pop_back();
    for(size_t i{}; i<asHex.length(); i++)
    {
        char c = asHex[i];
        if(c>='a' && c<='f') 
        {
            asHex[i]=c-'a'+'A'; 
            c = asHex[i];
        }
        
        
        if(!(c>='0' && c<='9') && !(c>='A' && c<='F'))
        {
            for(size_t i{}; i<asHex.length(); i++)
            {
                char c = asHex[i];
                if(c>='a' && c<='f') asHex[i]=c-'a'+'A';
            }
            globals::errorMessage+="Bad input to fromHex(), ("+asHex+")\n";
            globals::error=true;
            return NAN;
        }
    }
    T result{};
    T factor{1};
    for(size_t i{asHex.length()-1}; i+1>0; i--)
    {
        result+=hexDigitToDecimal(asHex[i])*factor.real();
        factor.real(factor.real()*16);
    }

    return result;
}

template <typename T>
T evaluateFromBin( const Token &arg)
{
    std::string asBin = arg.value();
    if(asBin.length()<1) return NAN;
    if(asBin.find("0b")==0) asBin.erase(0,2);
    if(asBin[asBin.length()-1]==')') asBin.pop_back();
    for (char c : asBin)
    {
        if(c!='0' && c!='1')
        {
            globals::errorMessage+="Bad input to fromBin(), ("+asBin+")\n";
            globals::error=true;
        }
    }
    T result{};
    T factor{1};
    for(size_t i{asBin.length()-1}; i+1>0; i--)
    {
        result+=(asBin[i]-'0')*factor.real();
        factor.real(factor.real()*2);
    }
    return result;
}

template <typename T>
T evaluateFromOct( const Token &arg)
{
    std::stringstream sstream;
    
    std::string asOct = arg.value();
    if(asOct.length()<1) return NAN;
    if(asOct[asOct.length()-1]==')') asOct.pop_back();
    for (char c : asOct)
    {
        if(!(c>='0' && c<='7'))
        {
            globals::errorMessage+="Bad input to fromOct(), ("+asOct+")\n";
            globals::error=true;
        }
    }
    T result{};
    T factor{1};
    for(size_t i{asOct.length()-1}; i+1>0; i--)
    {
        result+=(asOct[i]-'0')*factor.real();
        factor.real(factor.real()*8);
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T> [[nodiscard]]
T evaluateMean( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    T result{};
    std::vector<T> argVals;
    if(evaluateArgs(arg, xValue, argVals, fnCallParams)) return NAN; 
    for(size_t i{}; i<argVals.size(); i++)
    {
        result+=argVals.at(i);
    }
    result=result/T(argVals.size());

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateMedian( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    if constexpr((std::is_same_v<T,cpp_complex<maxPrecision>> || std::is_same_v<T,graphNum>))
    {    
        std::vector<T> argVals;
        if(evaluateArgs(arg, xValue, argVals, fnCallParams)) return NAN;

        std::sort(argVals.begin(), argVals.end(), [](T a, T b){return a.real()<b.real();});

        if(argVals.size()%2!=0) return argVals.at(argVals.size()/2);
        else return (argVals.at(argVals.size()/2-1)+argVals.at(argVals.size()/2))/T(2);
    }
    else return T();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateStdevp( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    if constexpr((std::is_same_v<T,cpp_complex<maxPrecision>> || std::is_same_v<T,graphNum>))
    {    
        std::vector<T> argVals;
        if(evaluateArgs(arg, xValue, argVals, fnCallParams)) return NAN;

        std::sort(argVals.begin()+1, argVals.end(), [](T a, T b){return a.real()<b.real();});

        if(argVals.size()<2)
        {
            return T(0);
        }
        T summedIntermediates{};
        for(size_t i{}; i<argVals.size(); i++)
        {
            summedIntermediates+=argVals.at(i);
        }
        const T mean{summedIntermediates/T(argVals.size())};
        T summed{};
        for(size_t i{}; i<argVals.size(); i++)
        {
            summed+=pow(argVals.at(i)-mean,2);
        }


        return sqrt(summed/T(argVals.size())); // Intellegre
    }
    else return T();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T> [[nodiscard]]
T evaluateLcm( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    // #error lcm infinite loops on table calculations
    if constexpr((std::is_same_v<T,cpp_complex<maxPrecision>> || std::is_same_v<T,graphNum>))
    {    
        std::ostringstream numberAsOSStream;
        if constexpr(std::is_same_v<T,cpp_complex<maxPrecision>>)
        {
            numberAsOSStream.precision(maxPrecision);
        }
        else numberAsOSStream.precision(std::numeric_limits<graphNumType>::digits10);
        std::vector<T> argVals;
        T tempValue{};
        T numLeft{};
        T numRight{};
        std::string numberAsString;
        if(evaluateArgs(arg, xValue, argVals, fnCallParams)) return NAN;
        for(size_t i{}; i<argVals.size(); i++)
        {
            argVals.at(i).imag(0);
        }
        if(globals::options.graph)
        {
            for(size_t i{}; i<argVals.size(); i++) argVals.at(i)=round(argVals.at(i).real());
        }
        for(size_t i{}; i<argVals.size(); i++)
        {
            if(argVals.at(i).real()<0) argVals.at(i).real(-argVals.at(i).real());
        }
        if(argVals.size()==1) return argVals.at(0);
        numLeft=argVals.at(0); //a
        numRight=argVals.at(1); //b
        while(argVals.at(1).real()!=0)
        {
            tempValue=argVals.at(1); //b
            argVals.at(1)=fmod(argVals.at(0).real(),argVals.at(1).real());
            argVals.at(0)=tempValue; 
        }
        argVals.at(0)=(numLeft*numRight)/argVals.at(0);
        argVals.erase(argVals.begin()+1);
        if(argVals.size()>=2)
        {
            numberAsOSStream<<"lcm(";
            for(size_t i{}; i<argVals.size(); i++)
            {
                numberAsOSStream<<argVals.at(i).real();
                numberAsOSStream<<',';
            }
            Token newArg{numberAsOSStream.str(),tokenType::LCM};
            argVals.at(0)=evaluateLcm(newArg,xValue, fnCallParams);
        }
        return argVals.at(0);
    }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T> [[nodiscard]]
T evaluateGcf( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    if constexpr((std::is_same_v<T,cpp_complex<maxPrecision>> || std::is_same_v<T,graphNum>))
    {    
        std::ostringstream numberAsOSStream;
        std::vector<T> argVals;
        T tempValue{};
        if(evaluateArgs(arg, xValue, argVals, fnCallParams)) return NAN;
        if(globals::options.graph)
        {
            for(size_t i{}; i<argVals.size(); i++) 
            {
                argVals.at(i).imag(round(argVals.at(i).imag()));
                argVals.at(i).real(round(argVals.at(i).real()));
            }
        }
        for(size_t i{}; i<argVals.size(); i++)
        {
            if(argVals.at(i).real()<0) argVals.at(i).real(-argVals.at(i).real());
            if(argVals.at(i).imag()<0) argVals.at(i).imag(-argVals.at(i).imag());
        }
        while(argVals.size()>=2)
        {
            while(argVals.at(1).real()>0 || argVals.at(1).imag()>0)
            {
                tempValue=argVals.at(1);
                if(argVals.at(1).real()>0)
                    argVals.at(1).real(fmod(argVals.at(0).real(),argVals.at(1).real()));
                if(argVals.at(1).imag()>0)
                    argVals.at(1).imag(fmod(argVals.at(0).imag(),argVals.at(1).imag()));
                argVals.at(0)=tempValue; 
            }
            argVals.erase(argVals.begin()+1);
        }
        return argVals.at(0);
    }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T=cpp_dec_float_100> [[nodiscard]]
T evaluateRndint( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    if constexpr((std::is_same_v<T,cpp_complex<maxPrecision>> || std::is_same_v<T,graphNum>))
    {    
        std::vector<T> argVals;
        std::string currentToken;
        if(evaluateArgs(arg, xValue, argVals, fnCallParams,2)) return NAN;
        
        if(argVals.size()==1) return argVals.at(0).real();

        if(argVals.at(0).real()!=argVals.at(0).real() || argVals.at(1).real()!=argVals.at(1).real()) return NAN; // Check forNAN

        if(argVals.at(0).real()>argVals.at(1).real()) std::swap(argVals.at(0), argVals.at(1));

        std::uniform_int_distribution<> intDist(static_cast<int>(argVals.at(0).real()),static_cast<int>(argVals.at(1).real()));
        return T(intDist(randomMt),0);
    }
}

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T> [[nodiscard]]
T evaluateRndsel( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    if constexpr((std::is_same_v<T,cpp_complex<maxPrecision>> || std::is_same_v<T,graphNum>))
    {    
        std::vector<T> argVals;
        if(evaluateArgs(arg, xValue, argVals, fnCallParams)) return NAN;
        if(argVals.size()==0) return NAN;
        std::uniform_int_distribution<size_t> intDist(0, argVals.size()-1);
        return argVals.at(intDist(randomMt));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T> [[nodiscard]]
T evaluateMax( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    std::vector<T> argVals;
    if(evaluateArgs(arg, xValue, argVals, fnCallParams)) return NAN;

    for(size_t i{}; i<argVals.size(); i++) if(argVals.at(i).imag()>argVals.at(0).imag()) argVals.at(0).imag(argVals.at(i).imag());
    for(size_t i{}; i<argVals.size(); i++) if(argVals.at(i).real()>argVals.at(0).real()) argVals.at(0).real(argVals.at(i).real());
    return argVals.at(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T> [[nodiscard]]
T evaluateDiff( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    if constexpr((std::is_same_v<T,cpp_complex<maxPrecision>> || std::is_same_v<T,graphNum>))
    {    
        std::vector<T> argVals;
        std::string currentToken;
        int nestingLevel{};
        T diffStepSize=static_cast<T>(globals::options.xStep);
        if(diffStepSize.real()<0.01) diffStepSize=0.01;
        for(size_t i{}; i<arg.value().length() && nestingLevel>=0 && argVals.size()<2; i++)
        {
            if(globals::error) return NAN;
            if(arg.value().at(i)=='(') nestingLevel++;
            else if(arg.value().at(i)==')') nestingLevel--;
            if(nestingLevel<0) break;
            if(!(arg.value().at(i)==',' && nestingLevel==0) && i<arg.value().length()) currentToken.push_back(arg.value().at(i));
            else
            {
                argVals.emplace_back(calculation<T>(getTokens(currentToken), xValue+diffStepSize, fnCallParams));
                argVals.emplace_back(calculation<T>(getTokens(currentToken), xValue-diffStepSize, fnCallParams));
                currentToken.clear();
                break;
            }
        }
        if(!currentToken.empty())
        {
            argVals.emplace_back(calculation<T>(getTokens(currentToken), xValue+diffStepSize, fnCallParams));
            argVals.emplace_back(calculation<T>(getTokens(currentToken), xValue-diffStepSize, fnCallParams));
            currentToken.clear();  
        }
        return (argVals.at(0)-argVals.at(1))/(diffStepSize.real()*T(2));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T> [[nodiscard]]
T evaluateMin( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    std::vector<T> argVals;
    if(evaluateArgs(arg, xValue, argVals, fnCallParams)) return NAN;

    for(size_t i{}; i<argVals.size(); i++) if(argVals.at(i).imag()<argVals.at(0).imag()) argVals.at(0).imag(argVals.at(i).imag());
    for(size_t i{}; i<argVals.size(); i++) if(argVals.at(i).real()<argVals.at(0).real()) argVals.at(0).real(argVals.at(i).real());
    return argVals.at(0);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateInrange( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    std::vector<T> argVals;
    std::string currentToken;
    if(evaluateArgs(arg, xValue, argVals, fnCallParams,3)) return NAN;
    if(argVals.size()<3) return NAN;
    auto &min=argVals[0];
    auto &expr=argVals[1];
    auto &max=argVals[2];
    
    return min.real()<=expr.real() && expr.real()<=max.real();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateMix( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    std::vector<T> argVals;
    std::string currentToken;
    if(evaluateArgs(arg, xValue, argVals, fnCallParams,3)) return NAN;
    
    else if(argVals.size()<3) return argVals.at(0);
    else if(argVals.at(2).real()>=1) return argVals.at(1);
    else if(argVals.at(2).real()<=0) return argVals.at(0);
    
    else return argVals.at(0)*(T(1,0)-argVals.at(2))+argVals.at(1)*argVals.at(2);
}

template <typename T> T evaluateAtan2( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    std::vector<T> argVals;
    std::string currentToken;
    if(evaluateArgs(arg, xValue, argVals, fnCallParams,2)) return NAN;

    if(argVals.size()<2) return NAN;

    return atan2(argVals.at(1).real(),argVals.at(0).real());

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateSabs( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    std::vector<T> argVals;
    std::string currentToken;
    if(evaluateArgs(arg, xValue, argVals, fnCallParams,2)) return NAN;
    
    if(argVals.size()<2) argVals.emplace_back(0.1); // Default argument
    T enumerator=argVals.at(1)-abs(argVals.at(0));
    if(enumerator.real()<0) enumerator.real(0);
    return abs(argVals.at(0))+(pow(enumerator,2)/(T(2)*argVals.at(1)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T> [[nodiscard]]
T evaluateIf( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    if constexpr((std::is_same_v<T,cpp_complex<maxPrecision>> || std::is_same_v<T,graphNum>))
    {    
        std::vector<T> argVals;
        std::string currentToken;
        std::vector<std::string> args;

        int nestingLevel{};
        for(size_t i{}; i<arg.value().length() && nestingLevel>=0 && argVals.size()<3; i++)
        {
            if(globals::error) return true;
            if(arg.value().at(i)=='(') nestingLevel++;
            else if(arg.value().at(i)==')') nestingLevel--;
            if(nestingLevel<0) break;
            if(!(arg.value().at(i)==',' && nestingLevel==0) && i<arg.value().length()) currentToken.push_back(arg.value().at(i));
            else
            {
                args.emplace_back(currentToken);
                currentToken.clear();
            }
        }
        if(!currentToken.empty()) 
        {
            args.emplace_back(currentToken);
        }
        if(args.size()<2) return NAN;
        if(calculation<T>(getTokens(args.at(0),false,false, fnCallParams), xValue, fnCallParams).real())
        {
            return calculation<T>(getTokens(args.at(1),false,false, fnCallParams), xValue, fnCallParams);
        }
        else
        {
            if(args.size()>2) return calculation<T>(getTokens(args.at(2),false,false, fnCallParams), xValue, fnCallParams);
            else return NAN;
        }
    }
    else return T();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateRound( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    std::vector<T> argVals;
    if(evaluateArgs(arg, xValue, argVals, fnCallParams,2)) return NAN;
    
    if(argVals.size()==1 || (argVals.at(1).real()<1 && argVals.at(1).imag()<1)) return T(round(argVals.at(0).real()),round(argVals.at(0).imag()));

    if(argVals.at(1).real()>maxPrecision) argVals.at(1).real(maxPrecision);
    else if(argVals.at(1).real()<0) argVals.at(1).real(0);

    if(argVals.at(1).imag()>maxPrecision) argVals.at(1).imag(maxPrecision);
    else if(argVals.at(1).imag()<0) argVals.at(1).imag(0);
    
    argVals.at(1)=T(floor(argVals.at(1).real()),floor(argVals.at(1).imag()));
    if(argVals.at(0).real()==argVals.at(0).real() && abs(argVals.at(0).real())!=INFINITY) argVals.at(0).real(argVals.at(0).real()-remainder(argVals.at(0).real(),1/pow(10,argVals.at(1).real())));
    if(argVals.at(0).imag()==argVals.at(0).imag() && abs(argVals.at(1).real())!=INFINITY) argVals.at(0).imag(argVals.at(0).imag()-remainder(argVals.at(0).imag(),1/pow(10,argVals.at(1).imag())));
    if(argVals.at(0).real()==-0) argVals.at(0).real(0);
    if(argVals.at(0).imag()==-0) argVals.at(0).imag(0);
    return argVals.at(0);

}
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateTrunc( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{

    std::vector<T> argVals;
    if(evaluateArgs(arg, xValue, argVals, fnCallParams,2)) return NAN;
    
    if(argVals.size()==1 || (argVals.at(1).real()<1 && argVals.at(1).imag()<1)) return T(trunc(argVals.at(0).real()),trunc(argVals.at(0).imag()));

    if(argVals.at(1).real()>maxPrecision) argVals.at(1).real(maxPrecision);
    else if(argVals.at(1).real()<0) argVals.at(1).real(0);

    if(argVals.at(1).imag()>maxPrecision) argVals.at(1).imag(maxPrecision);
    else if(argVals.at(1).imag()<0) argVals.at(1).imag(0);
    
    argVals.at(1)=T(floor(argVals.at(1).real()),floor(argVals.at(1).imag()));
    return argVals.at(0)-T(fmod(argVals.at(0).real(),1/pow(10,argVals.at(1).real())),fmod(argVals.at(0).imag(),1/pow(10,argVals.at(1).imag())));
    
    // return argVals.at(0)-fmod(argVals.at(0),1/pow(10,argVals.at(1)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateRoot( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    if constexpr((std::is_same_v<T,cpp_complex<maxPrecision>> || std::is_same_v<T,graphNum>))
    {    
        std::vector<T> argVals;
        std::string currentToken;
        if(evaluateArgs(arg, xValue, argVals, fnCallParams,2)) return NAN;
        
        if(argVals.size()==1) return sqrt(argVals.at(0)); // Default: sqrt()
        
        return pow(argVals.at(0), T(1)/argVals.at(1));
    }
    else return T();
}

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
T evaluateLog( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    if constexpr((std::is_same_v<T,cpp_complex<maxPrecision>> || std::is_same_v<T,graphNum>))
    {    
        std::vector<T> argVals;
        if(evaluateArgs(arg, xValue, argVals, fnCallParams,2)) return NAN;
        
        if(argVals.size()==1) argVals.emplace(argVals.begin(),10); // Default argument
        return log(argVals.at(1))/log(argVals.at(0));
    }
    else return T();
}

template <typename T> [[nodiscard]] T evaluateCustomFn( const Token &arg, const T &xValue, const std::unordered_map<std::string, std::string> &fnCallParams)
{
    if constexpr((std::is_same_v<T,cpp_complex<maxPrecision>> || std::is_same_v<T,graphNum>))
    {    
        size_t offset{};
        for(size_t i{MAX_KEYWORD_LENGTH}; i>0; i--)
        {
            if(globals::userFunctions.find(arg.value().substr(0,i))!=globals::userFunctions.end())
            {
                offset=i;
                break;
            }
        }

        
        if(offset==0) return NAN;
        Function fn=globals::userFunctions.find(arg.value().substr(0,offset))->second;
        std::vector<T> argVals;
        if(offset<arg.value().length()-1)
        {        
            Token argWithoutName{arg.value().substr(offset+1),tokenType::INVALID};
            if(evaluateArgs(argWithoutName, xValue, argVals, fnCallParams,fn.argc)) return NAN;
        }
        
        std::unordered_map<std::string,std::string> paramVariables{};

        std::ostringstream oss;
        for(size_t i{}; i<argVals.size() && i<fn.argNames.size(); i++)
        {
            std::string paramName=fn.argNames.at(i);
            for(size_t i{}; i<paramName.length(); i++)
            {
                char c=paramName.at(i);
                if(paramName.length()==1 && c=='x') break;
                if((c=='\t' || c=='\n' || c=='\\' || c=='(' || c==')' || c=='x') || c<=' ' || i>=MAX_KEYWORD_LENGTH)
                {
                    return NAN;
                }
            }
        
            oss.str("");
            if constexpr(std::is_same_v<T,graphNum>)
            {
                oss.precision(15);
            }
            else oss.precision(maxPrecision);

            oss<<argVals.at(i);
            if(paramVariables.find(paramName)!=paramVariables.end()) return NAN; // Bad definition, multiple params with same name
            else paramVariables.emplace(paramName, oss.str());
            
        }

        std::vector<Token> tokens=getTokens(globals::userFunctions.find(arg.value().substr(0,offset))->second.definition,false,true,paramVariables);
        if(argVals.size()>=fn.argc)
            return calculation<T>(tokens, xValue,paramVariables);
        else return NAN;

    }
    return NAN;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
using intType = int64_t;
template<typename T>
T evaluateBinary(Token &numberTokenLeft, Token &operation, Token &numberTokenRight, const T &xValue)
{
    
    if(globals::error) return NAN;
    T x{numberTokenLeft.number(xValue)};
    T y{numberTokenRight.number(xValue)};
    if(globals::opToID.find(operation.value())==globals::opToID.end()) return evaluateCustomBinary(numberTokenLeft, operation, numberTokenRight, xValue);
    using namespace globals;
    using enum idToOp;
    idToOp id=globals::opToID.find(operation.value())->second;

    // #error "Fix all the .real() where they're not sensible
    if constexpr((std::is_same_v<T,cpp_complex<maxPrecision>> || std::is_same_v<T,graphNum>))
        switch(id) // This goes against every principle.
        {
            case ADD: return x+y;
            case MULTIPLY: return x*y;
            case DIVIDE: return x/y;
            case FLOOR_DIV: return floor(x.real()/y.real());
            case EXPONENTIATE:
            {
                if(y!=floor(y.real()) || abs(y.real())>INT_MAX) return pow(x, y);
                else return pow(x,static_cast<int>(y.real()));
            }
            case MODULUS: return x-y*floor(x.real()/y.real());
            case LESS: return x.real()<y.real();
            case GREATER: return x.real()>y.real();
            case EQUALS: return x==y;
            case FMOD: return fmod(x.real(),y.real());
            case RMOD: return remainder(x.real(),y.real());
            case NPK: 
            {
                if(x.real()>=y.real()) return (tgamma(x.real()+1)/tgamma(x.real()-y.real()+1));
                else return NAN;
            }
            case NCK:
            {
                if(x.real()>=y.real()) return (tgamma(x.real()+1)/tgamma(x.real()-y.real()+1));
                else return NAN;
            }

            case AND: return x.real()&&y.real();
            case XOR: return (!x.real())!=(!y.real());
            case AROUND: return abs(x-y)<=globals::options.aroundTruthinessLeniency.real();
            case OR: return x.real()||y.real();
            case NOT_EQ: return x!=y;
            case GREATER_EQ: return x.real()>=y.real();
            case LESS_EQ: return x.real()<=y.real();
            
            case SHIFT_LEFT: return T(static_cast<intType>(x.real())<<static_cast<intType>(y.real()), static_cast<intType>(x.imag())<<static_cast<intType>(y.imag())); break;
            case SHIFT_RIGHT: return T(static_cast<intType>(x.real())>>static_cast<intType>(y.real()), static_cast<intType>(x.imag())>>static_cast<intType>(y.imag())); break;
            case BITWISE_AND: return T(static_cast<intType>(x.real())&static_cast<intType>(y.real()),static_cast<intType>(x.imag())&(static_cast<intType>(y.imag()))); break;
            case BITWISE_OR: return T(static_cast<intType>(x.real())|static_cast<intType>(y.real()),static_cast<intType>(x.imag())|(static_cast<intType>(y.imag()))); break;
            case BITWISE_XOR: return T(static_cast<intType>(x.real())^static_cast<intType>(y.real()),static_cast<intType>(x.imag())^(static_cast<intType>(y.imag()))); break;
            case SPACESHIP:
            {
                T result{};
                if(x.real()>y.real()) result.real(1);
                else if(x.real()==y.real()) result.real(0);
                else if(x.real()<y.real()) result.real(-1);
                else result.real(NAN);
                
                if(x.imag()>y.imag()) result.imag(1);
                else if(x.imag()==y.imag()) result.imag(0);
                else if(x.imag()<y.imag()) result.imag(-1);
                else result.imag(NAN);
                return result;
            }
            default: std::cerr<<"Declared, but undefined binary operator used... wonder how that happened.\n";
        }

    return NAN;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T>
T evaluateUnary(Token &numberToken, Token &operation, const T &xValue)
{
    if(globals::error) return NAN;
    if(globals::opToID.find(operation.value())==globals::opToID.end()) return evaluateCustomUnary(numberToken, operation, xValue);
    using namespace globals;
    using enum idToOp;
    idToOp id=globals::opToID.find(operation.value())->second;
    T x=numberToken.number(xValue);

    if constexpr((std::is_same_v<T,cpp_complex<maxPrecision>> || std::is_same_v<T,graphNum>)) // Jank
        switch(id) // :horror:
        {
            case NOT: return !x.real();
            case BITWISE_NOT: return ~static_cast<intType>(x.real());
            // case BITWISE_NOT: return T(~static_cast<intType>(x.real()),(~static_cast<intType>(x.imag())));
            case FACTORIAL: return tgamma(x.real()+1);
            case MINUS: return -x;
            case DOUBLE_FACTORIAL: 
            {
                // This sucks but apparently the radix of cpp_complex<MAX_PRECISION>, based on cpp_bin_float_100, is 0, so the constant thingy complains if I don't do it this way.
                if constexpr(std::is_same_v<T,cpp_complex<maxPrecision>>)
                {
                    return pow(T(2)/cpp_complex<maxPrecision>(boost::math::constants::pi<cpp_bin_float_100>(),0.),T(1)/4.*(T(1)-cos(cpp_complex<maxPrecision>(boost::math::constants::pi<cpp_bin_float_100>(),0.)*x)))*pow(2,x/T(2))*tgamma(x.real()/2+1);
                }
                else return pow(T(2)/boost::math::constants::pi<graphNumType>(),T(1)/4.*(T(1)-cos(boost::math::constants::pi<graphNumType>()*x)))*pow(2,x/T(2))*tgamma(x.real()/2+1);
            }
            case SINC: 
            {
                if(x.real()!=0) return sin(x)/x;
                else return 1;
            }
            case SINC_SQUARED: 
            {
                if(x.real()!=0) return pow(sin(x)/x,2);
                else return 1;
            }
            case EXP: 
            {
                Token e{tokenType::CONSTANT,"e"};
                return pow(e.number(xValue),x);            
            }
            case SIGN:
            {
                if(x.real()>0) return 1;
                if(x.real()<0) return-1;
                else return 0;
            }
            case SQRT: return sqrt(x);
            case CBRT: return pow(x,1./3);
            case QTRT: return pow(x,0.25);


            case SIN: return sin(x);
            case COS: return cos(x);
            case TAN: return tan(x);

            case SINH: return sinh(x);
            case COSH: return cosh(x);
            case TANH: return tanh(x);


            case ASIN: return asin(x);
            case ACOS: return acos(x);
            case ATAN: return atan(x);

            case ASINH: return asinh(x);
            case ACOSH: return acosh(x);
            case ATANH: return atanh(x);


            case SEC: return T(1)/cos(x);
            case CSC: return T(1)/sin(x);
            case COT: return T(1)/tan(x);

            case SECH: return T(1)/cosh(x);
            case CSCH: return T(1)/sinh(x);
            case COTH: return T(1)/tanh(x);

            case ASEC: return acos(T(1)/x);
            case ACSC: return asin(T(1)/x);
            case ACOT: return atan(T(1)/x);

            case ASECH: return acosh(T(1)/x);
            case ACSCH: return asinh(T(1)/x); // aschhschhshuhuschush
            case ACOTH: return atanh(T(1)/x);

            case PRIME:
            {
                x=floor(x.real());
                if(x.real()<=1) return false;
                else if(x.real()==2 || x.real()==3) return true;
                else if(fmod(x.real(),2)==0 || fmod(x.real(),3)==0) return false;
                for(size_t i{5}; i*i<=x.real(); i+=6)
                {
                    if(fmod(x.real(),i)==0 || fmod(x.real(),i+2)==0) return false;
                }
                return true;            
            }
            case LN: return log(x);
            case LN_SQUARED: return pow(log(x),2);
            case ABS: return abs(x);
            case FLOOR: return T(floor(x.real()),floor(x.imag()));
            case TRUNC: return T(trunc(x.real()),trunc(x.imag()));
            case CEIL: return T(ceil(x.real()),ceil(x.imag()));
            case BANKERS_ROUND:
            {
                if(x+0.5==round(x.real()) && fmod(floor(x.real()),2)==0)
                {
                    return floor(x.real());
                }
                else return round(x.real());            
            }
            case ROUND: return round(x.real());
            case SAT: return (1+abs(x)-abs(x-T(1)))/2;
            case RELU: return (x+abs(x))/T(2);
            case SSTEP: 
            {
                if(x.real()>1) return 1;
                else if(x.real()<0) return 0;
                return pow(x,2)*(T(3)-T(2)*x);            
            }
            case LGAMMA: return lgamma(x.real());
            case GAMMA: return tgamma(x.real());


            case SIN_SQUARED: return pow(sin(x),2);
            case COS_SQUARED: return pow(cos(x),2);
            case TAN_SQUARED: return pow(tan(x),2);

            case SINH_SQUARED: return pow(sinh(x),2);
            case COSH_SQUARED: return pow(cosh(x),2);
            case TANH_SQUARED: return pow(tanh(x),2);

            case ASIN_SQUARED: return pow(asin(x),2);
            case ACOS_SQUARED: return pow(acos(x),2);
            case ATAN_SQUARED: return pow(atan(x),2);

            case ASINH_SQUARED: return pow(asinh(x),2);
            case ACOSH_SQUARED: return pow(acosh(x),2);
            case ATANH_SQUARED: return pow(atanh(x),2);

            case SEC_SQUARED: return pow(T(1)/cos(x),2);
            case CSC_SQUARED: return pow(T(1)/sin(x),2);
            case COT_SQUARED: return pow(T(1)/tan(x),2);

            case SECH_SQUARED: return pow(T(1)/cosh(x),2);
            case CSCH_SQUARED: return pow(T(1)/sinh(x),2);
            case COTH_SQUARED: return pow(T(1)/tanh(x),2);

            case ASEC_SQUARED: return pow(acos(T(1)/x),2);
            case ACSC_SQUARED: return pow(asin(T(1)/x),2);
            case ACOT_SQUARED: return pow(atan(T(1)/x),2);

            case ASECH_SQUARED: return pow(acosh(T(1)/x),2);
            case ACSCH_SQUARED: return pow(asinh(T(1)/x),2);
            case ACOTH_SQUARED: return pow(atanh(T(1)/x),2);

            case REAL: return x.real();
            case IMAG: return x.imag();
            case ARG: return arg(x);
            case NORM: return norm(x);
            case CONJ: return conj(x);
            case PROJ: return proj(x);
            case CIS: return cos(x)+T(0,1)*sin(x);
            case CIS_SQUARED: return pow(cos(x)+T(0,1)*sin(x),2);
            case CAS: return sin(x)+cos(x);
            case CAS_SQUARED: return pow(sin(x)+cos(x),2);

            
            default: std::cout<<"Declared, but undefined unary operator used\n";
        }
    return T();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The GOAT
// Lost plot relevancy as of recent
inline bool isRealNumber(const std::string &input, bool disallowSpecials)
{
    if(input.empty()) return false;
    if(!disallowSpecials)
    {
        if(input=="inf") return true;
        if(input=="-inf") return true;
        if(input=="nan") return true;
        if(input=="-nan") return true;
    }
    if(input=="-") return false;
    if(input=="e") return false;

    for(size_t i{}; i<input.length(); i++) if((input.at(i)<'0' || input.at(i)>'9') && 
                                               input.at(i)!='e' && 
                                               input.at(i)!='.' &&
                                               input.at(i)!='+' &&
                                               input.at(i)!='-') return false;
    size_t dotCount{};
    size_t eCount{};
    size_t exponentLength{};
    bool seenMinus{};
    

    for(size_t i{}; i<input.length() && eCount<2; i++)
    {
        if(eCount) exponentLength++;
        if(exponentLength>9) return false; // Avoids a really pissy crash.
        if((seenMinus && input.at(i)=='-')||(i>0 && input.at(i)=='-' && input.at(i-1)!='e')) return false;
        if(input.at(0)=='-' && i==0 && input.length()>1)
        {
            seenMinus=true;
            continue;
        }
        if(input.at(i)=='e') 
        {
            if(i+2<input.length() && input.at(i)=='e' && (input.at(i+1)=='+' || input.at(i+1)=='-') && std::isdigit(input.at(i+2))) i+=2;
            eCount++;
        }
        if(input.at(i)=='.')
        {
            dotCount++;
            if(i==input.length()-1) return false;
            if(eCount) 
            {
                if(!globals::passedCalculationsFile) globals::errorMessage+="Number \""+input+"\" can only have an integer exponent\n";
                globals::error=true;
                return false;
            }
            if(dotCount>1)
            {
                if(!globals::passedCalculationsFile) globals::errorMessage+="Number \""+input+"\" has multiple decimal points\n";
                globals::error=true;
                return false;
            }
        }

        if(!isNumberPart(input.at(i))) return false;
    }
    if(eCount>1) return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline bool isNumberPart(const char input)
{
    return (input>='0' && input<='9') || input=='.' || input=='e';
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


inline long long unguardedGcd(long long a, long long b)
{
    if(b==0) return a;
    else return unguardedGcd(b,a%b);
}

inline Frac decimalToFraction(cpp_complex<maxPrecision> numerator, size_t precision)
{
    const bool negative{numerator.real()<0};
    if(negative) numerator=-numerator;
    std::string fraction;
    std::string number;
    std::ostringstream asOSStream;
    asOSStream.precision(15);
    if constexpr (std::is_same<cpp_complex<maxPrecision>,cpp_complex<maxPrecision>>()) asOSStream.precision(precision);
    asOSStream<<numerator;
    number=asOSStream.str();
    cpp_complex<maxPrecision> denominator{1};

    while(numerator!=round(numerator.real()) && numerator==numerator)
    {
        numerator*=10;
        denominator*=10;
    }

    size_t length=number.substr(number.find('.')+1).length()-1;
    bool hasTriedTwice{};
    retry:
    std::string pattern=number.substr(number.find('.')+1,length/2.f);
    size_t occurences{};
    for(size_t i{number.find('.')+1}; i<number.length() && !pattern.empty() && length>=11; i++)
    {
        if(number.find(pattern,i)==i)
        {
            occurences++;
            i+=pattern.length()-1;
        }
    }
    if(occurences<=1 && length>=12 && !hasTriedTwice)
    {
        length-=1;
        hasTriedTwice=true;
        goto retry;
    }

    
    if(occurences>1)
    {
        numerator=static_cast<cpp_complex<maxPrecision>>(number.substr(0,number.find('.')));
        denominator=pow(10,pattern.length())-1;
        numerator=(numerator*denominator)+std::stoll(pattern);
    }
    long long numeratori=static_cast<long long>(numerator);
    long long denominatori=static_cast<long long>(denominator);
    long long gcd=unguardedGcd(numeratori,denominatori);


    numerator/=gcd;
    denominator/=gcd;

    if(negative) numerator=-numerator;

    return Frac(static_cast<graphNumType>(numerator),static_cast<graphNumType>(denominator));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool isValidName(std::string &name, bool modifyName=false);


inline bool isValidName(std::string &name, bool modifyName)
{
    if(name.empty()) return false;
    if(std::isdigit(name[0]))
    {
        if(!modifyName) return false;
        name.erase(0,1);
    }
    bool allDigits{true};
    for(size_t i{}; i<name.length(); i++)
    {
        char c=name.at(i);
        if(!std::isdigit(c)) allDigits=false;
        if((c=='\t' || c=='\n' || c=='\\' || c=='(' || c==')' || c=='\'' || c=='#') || c<=' ' || i>=MAX_KEYWORD_LENGTH)
        {
            if(!modifyName) return false;
            name.erase(i--,1);
        }
    }
    if(globals::symbols.find(name)!=globals::symbols.end() || name=="h*")
    {
        return false;
    }
    if(allDigits) return false;
    return true;
}


inline bool isEmptyEquation(std::string equation)
{
    equation=replaceUnicode(equation);
    for(size_t i{}; i<equation.length(); i++)
    {
        if(!(lessetB::isValidInput(equation.at(i)))) equation.erase(equation.begin()+i--);
    }
    return !equation.length(); // True if 0
}

inline void addSeparatorAroundDigitGroups(std::string &num, char decimalSymbol, const std::string &separator)
{
    if(num=="Not a Number" || num=="nan")
    {
        return;
    }
    if(num.find(decimalSymbol)==std::string::npos && num.find(',')==std::string::npos)
    {
        size_t j{};
        for(size_t i{num.length()-1}; i>0; i--)
        {
            if(i==1 && num.at(0)=='-') continue;
            if(j%3==2 && std::isdigit(num[i]))
            {
                num.insert(i,separator);
                if(i>2)
                    i-=separator.length()-1;
                else break;
            }
            j++;
        }
    }
    else if(num.at(0)!='(' || num.find(',')==std::string::npos)
    {
        {
            size_t j{};
            for(size_t i{num.find(decimalSymbol)-1}; i>0; i--)
            {
                if(i==1 && num.at(0)=='-') continue;
                if(j%3==2 && std::isdigit(num[i]))
                {
                    num.insert(i,separator);
                    if(i>2)
                        i-=separator.length()-1;
                    else break;
                }
                j++;
            }
        }
        {
            size_t j{1};
            for(size_t i{num.find(decimalSymbol)+1}; i<num.length(); i++)
            {
                if(num.find("e")<i) break;
                if(i==1 && num.at(0)=='-') continue;
                if(j%4==0 && std::isdigit(num[i]))
                {
                    num.insert(i,separator);
                    i+=separator.length()-1;
                }
                j++;
            }
        }
    }
    else
    {
        std::string result="(";
        for(size_t i{}; i<2; i++)
        {
            std::string curr;
            if(i==0) curr=num.substr(1,num.find(',')-1);
            else curr=num.substr(1);
            num.erase(0,num.find(','));
            if(curr.find("Not a Number")!=std::string::npos || curr.find("nan")!=std::string::npos)
            {
                result+="Not a Number";
                if(i==0) result+=",";
                continue;
            }

            if(curr.find("inf")!=std::string::npos || curr.find("∞")!=std::string::npos)
            {
                result+="∞";
                if(i==0) result+=",";
                continue;
            }
            
            if(curr.find(decimalSymbol)==std::string::npos)
            {
                size_t j{};
                for(size_t i{curr.length()-1}; i>0; i--)
                {
                    if(i==1 && curr.at(0)=='-') continue;
                    if(j%3==2 && std::isdigit(num[i]))
                    {
                        curr.insert(i,separator);
                        if(i>3)
                            i-=separator.length()-1;
                        else break;
                    }
                    j++;
                }
            }
            else
            {
                {
                    size_t j{};
                    for(size_t i{curr.find(decimalSymbol)-1}; i>0; i--)
                    {
                        if(i==1 && curr.at(0)=='-') continue;
                        if(j%3==2 && std::isdigit(num[i]))
                        {
                            curr.insert(i,separator);
                            if(i>3)
                                i-=separator.length()-1;
                            else break;
                        }
                        j++;
                    }
                }
                {
                    size_t j{1};
                    for(size_t i{curr.find(decimalSymbol)+1}; i<curr.length() && curr.at(i)!=')'; i++)
                    {
                        if(curr.find("e")<i) break;
                        if(i==1 && curr.at(0)=='-') continue;
                        if(j%4==0 && std::isdigit(num[i]))
                        {
                            curr.insert(i,separator);
                            i+=separator.length()-1;
                        }
                        j++;
                    }
                }
            }
            
            result+=curr;
            if(i==0) result+=",";
        }
        if(result.at(result.length()-1)!=')') result+=")";
        num=result;
    }
}

[[nodiscard]] inline std::string formatNumber(std::string result, const Options &options, bool noFractions, const bool noConstants)
{
    size_t leadingSpaces{};
    size_t trailingSpaces{};
    
    while(result.length()>0 && result[0]==' ')
    {
        leadingSpaces++;
        result.erase(0,1);
    }
    while(result.length()>0 && result[result.length()-1]==' ')
    {
        trailingSpaces++;
        result.pop_back();
    }
    
    if(result.length()<3) 
    {
        if(globals::useDecimalComma && (result.find('.')!=std::string::npos || result.find(',')!=std::string::npos) && !globals::error)
        {
            for(size_t i{}; i<result.length(); i++) if(result.at(i)==',') result.at(i)=';';
            for(size_t i{}; i<result.length(); i++) if(result.at(i)=='.') result.at(i)=',';
        }
        return result;
    }
    std::ostringstream resultAsOSStream;
    resultAsOSStream<<result;
    bool isKnownConstant{};
    bool isFraction{};
    // Show results equal to a constant as that constant
    if(!noConstants && options.prettyPrinting && globals::valueToConstant.find(resultAsOSStream.str())!=globals::valueToConstant.end())
    {
        result=globals::valueToConstant.find(resultAsOSStream.str())->second;
        isKnownConstant=true;
    }
    else if(!noConstants && options.prettyPrinting && result.length()<20 && globals::valueToConstantLenient.find(resultAsOSStream.str())!=globals::valueToConstantLenient.end())
    {
        result=globals::valueToConstantLenient.find(resultAsOSStream.str())->second;
        isKnownConstant=true;
    }
    if(result.at(0)=='-')
    {

        // -0.00 -> 0.00
        std::string trimmedSpaces=result;
        for(int i{}; i<trimmedSpaces.length() && i+1>0; i++)
        {
            if(std::isspace(trimmedSpaces.at(i))) trimmedSpaces.erase(i--,1);
        }
        if(options.prettyPrinting && isRealNumber(trimmedSpaces))
        {
            double asNum{};
            std::from_chars(result.c_str(),result.c_str()+result.length(),asNum);
            if(asNum==0) result.erase(0,1);
        }
        if(!noConstants && options.prettyPrinting && resultAsOSStream.str().length()>1 && globals::valueToConstant.find(resultAsOSStream.str().substr(1))!=globals::valueToConstant.end())
        {
            result=globals::valueToConstant.find(resultAsOSStream.str().substr(1))->second;
            result.insert(0,"-");
            isKnownConstant=true;
        }
        else if(!noConstants && options.prettyPrinting && resultAsOSStream.str().length()>1 && result.length()<20 && globals::valueToConstantLenient.find(resultAsOSStream.str().substr(1))!=globals::valueToConstantLenient.end())
        {
            result=globals::valueToConstantLenient.find(resultAsOSStream.str().substr(1))->second;
            result.insert(0,"-");
            isKnownConstant=true;
        }
    }

    // Check for fractions
    if( options.prettyPrinting && !noFractions && 
        isRealNumber(resultAsOSStream.str(),true) && 
        !isKnownConstant && 
        resultAsOSStream.str().find('e')==std::string::npos)
    {
        Frac frac=decimalToFraction(static_cast<cpp_complex<maxPrecision>>(resultAsOSStream.str()));
        
        std::string currentResult=resultAsOSStream.str();
        resultAsOSStream.str("");
        resultAsOSStream<<frac.numer<<'/'<<frac.denom;
        static_cast<cpp_complex<maxPrecision>>(currentResult).real();

        // Just try a couple times
        for(size_t i{10}; 
            (frac.numer==INFINITY || abs(calculation<cpp_complex<maxPrecision>>(getTokens(resultAsOSStream.str(),false,true), NAN)-static_cast<cpp_complex<maxPrecision>>(currentResult)).real()>=cpp_complex<maxPrecision>(0.000000000000000000000000000000001).real()) && 
            i<=20; i++)
        {
            frac=decimalToFraction(static_cast<cpp_complex<maxPrecision>>(currentResult),i);
            resultAsOSStream.str("");
            resultAsOSStream<<frac.numer<<'/'<<frac.denom;
        }

        if( frac.denom!=1 &&
            abs(frac.numer)!=INFINITY &&
            abs(calculation<cpp_complex<maxPrecision>>(getTokens(resultAsOSStream.str(),false,true), NAN)-static_cast<cpp_complex<maxPrecision>>(currentResult).real())<cpp_complex<maxPrecision>(0.000000000000000000000000000000001).real() ) 
        {
            isFraction=true;
            result=resultAsOSStream.str();
        }
    }
    if(!isKnownConstant && options.prettyPrinting && !isFraction && result.find('e')==std::string::npos)
    {
        addSeparatorAroundDigitGroups(result, '.', globals::separator);
    }
    if(globals::useDecimalComma && (result.find('.')!=std::string::npos || result.find(',')!=std::string::npos) && !globals::error)
    {
        for(size_t i{}; i<result.length(); i++) if(result.at(i)==',') result.at(i)=';';
        for(size_t i{}; i<result.length(); i++) if(result.at(i)=='.') result.at(i)=',';
    }
    for(size_t i{1}; i<1000 && i<result.length()-2 && result.length()>2; i++)
    {
        if(std::isdigit(result.at(i-1)) && result.at(i)=='e' && (result.at(i+1)=='-' || result.at(i+1)=='+') && std::isdigit(result.at(i+2)))
        {
            result.replace(i,(result.at(i+1)=='-')+2*(result.at(i+1)=='+'),"×10^");
            i+=sizeof("×10^")+result.at(i+1)=='-';
        }
    }
    if(leadingSpaces!=0)
    {
        std::string spaces;
        spaces.reserve(leadingSpaces);
        for(size_t i{}; i<leadingSpaces; i++)
        {
            spaces.push_back(' ');
        }
        result.insert(0, spaces);
        spaces.clear();
        for(size_t i{}; i<trailingSpaces; i++)
        {
            spaces.push_back(' ');
        }
        result.append(spaces);
    }
    return result;
}

template<typename T>
inline T sstepCmpl(T x)
{
    if(x.real()>1) return 1;
    else if(x.real()<0) return 0;
    return pow(x,2)*(T(3)-T(2)*x);  
}

template<typename T>
inline T sstep(T x)
{
    if(x>1) return 1;
    else if(x<0) return 0;
    return pow(x,2)*(T(3)-T(2)*x);  
}

template<typename T>
inline T normalize(T val, T max, T min)
{
    return(val-min)/(max-min);
}

template<typename T>
inline T round(T num, size_t digits)
{
    return num-remainder(num,1/pow(10,digits));
}

/*
3+(pi/root(2+4,10-2))-25x

3: Number                                              -> NUMBER
+: BinaryOp                                            -> OPERATOR
(pi/root(2+4,10-2)): SubExpr                           -> SUBEXPR
    pi: Constant (Will later be replaced by Number)    -> CONSTANT
    /: BinaryOp                                        -> OPERATOR
    root(2+4,10-2): Root                               -> SUBEXPR
        argVals:
            [0]
                2: Number                              -> NUMBER   
                +: BinaryOp                            -> OPERATOR
                4: Number                              -> NUMBER
            [1]
                10: Number                             -> NUMBER    
               -: UnaryOp (Will later be treated as +-)-> OPERATOR
                2: Number                              -> NUMBER
-:UnaryOp (Will later be treated as +-)                -> OPERATOR
25:Number                                              -> NUMBER
x:Variable (Will later be replaced by Number)          -> NUMBER

*/

}