/*
    This program is based off of the GLFW-Vulkan example for ImGui.
    Look at the include paths below for some guidance on how you should structure things.
    First bit of my code is at line 61
*/
#include<bitset>
#include <cassert>
#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>
#include <misc/cpp/imgui_stdlib.h>
#include <implot.h>
#include <implot_internal.h>
#include "NotoSansMathRegular.hpp"
#include "imgui_styles.h"
#include "lesset.hpp"
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <ios>
#include <stdio.h>          
#include <stdlib.h>         
#include <string>
#include <map>
#include <type_traits>
#include <unordered_map>
#include <chrono>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <fstream>
#include <nfd.hpp>

// Volk headers
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// #define APP_USE_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define APP_USE_VULKAN_DEBUG_REPORT
static VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
#endif

// Data
static VkAllocationCallbacks*   g_Allocator = nullptr;
static VkInstance               g_Instance = VK_NULL_HANDLE;
static VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
static VkDevice                 g_Device = VK_NULL_HANDLE;
static uint32_t                 g_QueueFamily = (uint32_t)-1;
static VkQueue                  g_Queue = VK_NULL_HANDLE;
static VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;

static ImGui_ImplVulkanH_Window g_MainWindowData;
static uint32_t                 g_MinImageCount = 2;
static bool                     g_SwapChainRebuild = false;

enum class sliderAnimMode
{
    TO_FRO,
    TO,
    TO_FRO_SMOOTH,
};

struct SliderData
{
    std::array<float,4> nums{0,0,0,1};
    sliderAnimMode animMode{};
    bool animate{};
    bool movementDirection{};
};

struct Instance
{
    std::string name;
    lessetB::CustomSymbols customSymbols;
    std::unordered_map<std::string, char> protectedFunctionDefinitions; // Protects some functions from being undef'd
    std::map<std::string,SliderData> sliders;
    std::string lastScriptOutput;
    bool hasRunScript{};

};

struct SpecialPoint
{
    std::string eq;
    ImVec4 col;
    double x{NAN};
    double y{NAN};
};

inline bool containsX(const std::string &input, Instance &instance)
{
    return containsX(input, instance.customSymbols);
}

inline bool containsX(const std::string &input, lessetB::CustomSymbols &customSymbols)
{
    // assert(input.find(' ')==std::string::npos && input.find('\t')==std::string::npos && input.find('\n')==std::string::npos);
    if(input.length()>=1 && input.at(0)=='x') return true;
    for(size_t i{}; i<input.length(); i++)
    {
        std::string candidate;
        for(size_t j{MAX_KEYWORD_LENGTH}; j>0; j--)
        {
            if(j>input.length()-1) j=input.length()-i;
            candidate=input.substr(i,j);
            if(lessetB::globals::symbols.find(candidate)!=lessetB::globals::symbols.end() || 
               customSymbols.m_variables.find(candidate)!=customSymbols.m_variables.end() ||
               customSymbols.m_operations.find(candidate)!=customSymbols.m_operations.end() ||
               lessetB::globals::multiArgFunctions.find(candidate)!=lessetB::globals::multiArgFunctions.end())
            {
                if(lessetB::globals::multiArgFunctions.find(candidate)!=lessetB::globals::multiArgFunctions.end())
                {
                    
                    if((i+j<input.length() && input.at(i+j)=='(')) 
                    {
                        i+=j;
                        break;
                    }
                    else if((i+j+1>=input.length())) break;
                }
                if(candidate!="x")
                {
                    i+=j-1;
                    break;
                }
                else return true;
            }
        }
    }
    return false;
}

enum class symbolCreationError : char8_t
{
    OKAY,
    BAD_SIGNATURE,
    BAD_DEFINITION,
    BAD_NAME,
    DOUBLED_CUSTOM_NAME,
    BAD_PARAMS,
};

inline lessetB::Function getFnFromArgs(const std::string &def, std::string &sig, Instance &instance)
{
    if(lessetB::isEmptyEquation(def) || containsX(def,instance)) return lessetB::Function();
    std::string argName;
    std::vector<std::string> argNames;
    if(sig.find('(')==std::string::npos || sig.find(')')==std::string::npos) return lessetB::Function();

    {
        size_t lparenCount{};
        size_t rparenCount{};
        for(size_t i{}; i<sig.length(); i++)
        {
            if(sig.at(i)=='(') lparenCount++;
            else if(sig.at(i)==')') rparenCount++;
        }
        if(lparenCount!=1 || rparenCount!=1) return lessetB::Function();
    }

    for(size_t i{sig.find('(')+1}; i<sig.length() && sig.at(i)!=')'; i++)
    {
        char c=sig.at(i);
        if(!lessetB::isValidInput(c))
        {
            sig.erase(i,1);
            continue;
        }
        if(c!=',') argName.push_back(c);
        else 
        {
            argNames.emplace_back(argName);
            argName.clear();
        }
    }
    if(!argName.empty()) argNames.emplace_back(argName);
    for(std::string name : argNames)
    {
        if(lessetB::globals::symbols.find(name)!=lessetB::globals::symbols.end() || lessetB::globals::multiArgFunctions.find(name)!=lessetB::globals::multiArgFunctions.end())
        {
            return lessetB::Function();
        }
    }
    lessetB::Function fn{def,argNames,argNames.size()};
    return fn;
}

inline lessetB::Function getFnFromSignature(const std::string &sig, Instance &instance)
{
    {
        size_t lparenCount{};
        size_t rparenCount{};
        for(size_t i{}; i<sig.length() && sig.at(i)!='='; i++)
        {
            if(sig.at(i)=='(') lparenCount++;
            else if(sig.at(i)==')') rparenCount++;
        }
        if(lparenCount!=1 || rparenCount!=1) return lessetB::Function();
    }

    if(sig.find('=')==std::string::npos || 
       lessetB::isEmptyEquation(sig.substr(sig.find('=')+1)) || 
       containsX(sig.substr(sig.find('=')+1), instance)) return lessetB::Function();
    
    std::string argName;
    std::vector<std::string> argNames;
    for(size_t i{sig.find('(')+1}; i<sig.length(); i++)
    {
        char c=sig.at(i);
        if(c==')') break;
        if(!lessetB::isValidInput(c)) return lessetB::Function();
        if(c!=',') argName.push_back(c);
        else
        {
            argNames.emplace_back(argName);
            argName.clear();
        }
    }
    if(!argName.empty()) argNames.emplace_back(argName);
    for(std::string name : argNames)
    {
        if(lessetB::globals::symbols.find(name)!=lessetB::globals::symbols.end() || lessetB::globals::multiArgFunctions.find(name)!=lessetB::globals::multiArgFunctions.end())
        {
            return lessetB::Function();
        }
    }
    
    lessetB::Function fn{sig.substr(sig.find('=')+1),argNames,argNames.size()};
    return fn;
}

inline symbolCreationError getOperationFromArgs(std::string &sig, const std::string &def, lessetB::pass precedence, lessetB::operatorAttributes attribs, Instance &instance)
{
    using enum symbolCreationError;
    attribs=attribs&(lessetB::U_PREFIX|lessetB::U_POSTFIX|lessetB::L_ASSOCIATIVE|lessetB::R_ASSOCIATIVE);
    if(lessetB::isEmptyEquation(def) || containsX(def,instance)) return BAD_DEFINITION;

    std::string argName;
    std::array<std::string,2> argNames;
    if(sig.find('(')==std::string::npos || sig.find(')')==std::string::npos) return BAD_SIGNATURE;

    {
        size_t lparenCount{};
        size_t rparenCount{};
        for(size_t i{}; i<sig.length(); i++)
        {
            if(sig.at(i)=='(') lparenCount++;
            else if(sig.at(i)==')') rparenCount++;
        }
        if(lparenCount!=1 || rparenCount!=1) return BAD_SIGNATURE;
    }

    size_t argc{};
    for(size_t i{sig.find('(')+1}; i<sig.length() && sig.at(i)!=')'; i++)
    {

        char c=sig.at(i);
        if(!lessetB::isValidInput(c))
        {
            sig.erase(i,1);
            continue;
        }
        if(c!=',') argName.push_back(c);
        else 
        {
            argNames[argc]=argName;
            argc++;
            argName.clear();
        }
    }
    if(!argName.empty()) 
    {
        argNames[argc]=argName; 
        argc++;
    }
    if(argc==2)
    {
        attribs=attribs|lessetB::BINARY;
        attribs=attribs&(lessetB::BINARY|lessetB::L_ASSOCIATIVE|lessetB::R_ASSOCIATIVE);
    }
    else if (attribs&lessetB::BINARY) attribs=lessetB::operatorAttributes(attribs-lessetB::BINARY);
    for(std::string name : argNames)
    {
        if(lessetB::globals::symbols.find(name)!=lessetB::globals::symbols.end() || lessetB::globals::multiArgFunctions.find(name)!=lessetB::globals::multiArgFunctions.end())
        {
            return BAD_PARAMS;
        }
    }

    std::string name = sig.substr(0,sig.find('('));

    lessetB::Operation op{def,precedence,attribs,argNames};
    if(instance.customSymbols.m_variables.find(name)!=instance.customSymbols.m_variables.end()) return DOUBLED_CUSTOM_NAME;
    if(instance.customSymbols.m_operations.find(name)!=instance.customSymbols.m_operations.end()) instance.customSymbols.m_operations.erase(name);

    if(lessetB::globals::symbols.find(name)!=lessetB::globals::symbols.end())
        return BAD_NAME;    

    instance.customSymbols.m_operations.emplace(name,op);
    return OKAY;
}

inline bool addVariable(const std::pair<std::string,std::string> &newVariable, Instance &instance)
{
    auto &variableContainer = instance.customSymbols.m_variables;
    using namespace lessetB;
    globals::options= Options{false,
                                    0,
                                    0,
                                    0,
                                    0,
                                    false,
                                    0,
                                    "",
                                    0,
                                    ""};
    std::string name=newVariable.first;
    if(name.size()>0 && std::isdigit(name.at(0))) return true;
    if(!lessetB::isValidName(name)) return true;
    globals::oss.precision(maxPrecision);
    globals::oss.str("");
    if(globals::symbols.find(name)!=globals::symbols.end()) return true;
    
    cpp_complex<maxPrecision> value=calculation<cpp_complex<maxPrecision>>(getTokens(newVariable.second,false,true),NAN);
    if(isEmptyEquation(newVariable.second)) value=cpp_complex<maxPrecision>(0,0);
    
    globals::oss<<value;
    if(globals::oss.str().find('(')==std::string::npos)
    {
        globals::oss.str("("+globals::oss.str()+",0)");
    }
    if(instance.customSymbols.m_operations.find(newVariable.first)!=instance.customSymbols.m_operations.end()) return true;
    if(variableContainer.find(name)!=variableContainer.end()) variableContainer.find(name)->second=globals::oss.str();
    else variableContainer.emplace(name, globals::oss.str());
    globals::tokenMemory.clear();
    return false;
}

struct GraphEquation
{
    std::string eq;
    std::pair<std::string,lessetB::Function> fn;
    ImVec4 col=ImVec4(rand()/static_cast<float>(RAND_MAX)+.3,rand()/static_cast<float>(RAND_MAX)+.3,rand()/static_cast<float>(RAND_MAX)+.3,204/255.f);
    std::chrono::nanoseconds timePerPoint;
    bool disabled{};
    
    void setTimePerPoint(Instance &instance)
    {
        lessetB::globals::options =  lessetB::Options{true,0,1,1,0,false,true,fn.first,false,lessetB::globals::options.ans};
        std::string s,s2;
        std::chrono::nanoseconds delta{};
        for(size_t i{}; i<4; i++)
        {
            auto beginning = std::chrono::steady_clock::now();
            std::unordered_map<std::string, std::string> um{};
            std::vector<lessetB::Token> tokens = lessetB::getTokens(lessetB::cleanInput(lessetB::replaceUnicode(eq)));
            graphNum empty{};
            calculation<graphNum>(tokens,3.14+i);
            auto end = std::chrono::steady_clock::now();
            delta += std::chrono::duration_cast<std::chrono::nanoseconds>(end-beginning);
        }

        lessetB::globals::options={false,0,0,1,0,false,true,"",false,lessetB::globals::options.ans};
        delta/=4;
        timePerPoint=delta;
        // std::cout<<delta<<'\n';
    }
};


#define CLEAR_BRIGHTNESS 0.05f       // 0 ≤ CLEAR_BRIGHTNESS ≤ 1
#define TRIMMED_DECIMAL_PLACES 2     // Decimal places trimmed from coordinates display for special points
#define HIGH_PRECISION_DRAW_DELAY 50 // Delay in frames between no movement and recalculating of graphs at full precision
#define MANY_GRAPHS 100              // When the checkbox allowing the disabling of all graphs is added
#define MAX_COMMANDS 100000          // Hard cap for scripts
#define MAX_GRAPH_POINTS_BASE 12000   // Medium graphing precision, other precisions are based off of this one
#define MAIN_INSTANCE 0
#define ROW_HEIGHT 26
// In Nanoseonds, the target around which the amount of points for a certain graph should be oriented.
// In essence, laggier graphs get less points, as their time per point is higher, how aggressively depends on this.
#define TARGET_PER_POINT_TIME 30000.
#define MAX_POINT_REDUCTION 8

// I can't describe these very well. They're used in finding discontinuities.
#define MAX_CHANGE_FACTOR_FIRST 6 
#define MAX_CHANGE_FACTOR_SECOND 3 



void updateSliders(size_t &timeStationary, std::vector<Instance> &instances, bool &recalculateGraphs, size_t &currInst);

bool isNoisy(const std::vector<double> &pointsX, const std::vector<double> &pointsY, size_t i, int maxIndividualGraphPointsMultiplier);
bool addVariable(Instance &data,const std::pair<std::string,std::string> &newVariable);
int addClosingParentheses(std::string &equation);
void addSeparatorsAfterFunctions(std::string &input, const Instance &instance, const std::string separator);
void cleanWhitespace(std::string &input);
bool isPreviousPlotHidden();
bool isCurrentPlotHidden();
void displayLimitsUI(const ImPlotRect& limits, lessetB::Options &options);
void displayAxisUI();
void displayGridUI(const ImPlotRect &limits);
void runScript(std::vector<GraphEquation> &graphEquations, lessetB::Options &options, std::vector<Instance> &instances, bool &recalculateGraphs,
                  size_t &currInst, std::string &filePath, bool restricted=false, bool writeOutput=true);
void writeSave(lessetB::Options &options);
void writeGraphsScript(const std::string &path,const std::vector<GraphEquation> &graphEquations, const std::string &filename);
void readSave(lessetB::Options &options);
bool ImVec4Equal(const ImVec4 &a, const ImVec4 &b, bool cmpAlpha=true);
void replaceWithUnicode(std::string &input, 
                        const std::unordered_map<std::string, lessetB::tokenType> &symbols, 
                        const std::unordered_map<std::string, lessetB::tokenType> &multiArgFunctions,
                        const lessetB::CustomSymbols &customSymbols);


void displayMathConstantsMenu(std::string &equation);
void displayPhysicsConstantsMenu(std::string &equation);
void displayHyperbolicHelpMenu(std::string &equation);
void displayTrigHelpMenu(std::string &equation, int &tooltipRnd);
void displayCalculusHelpMenu(std::string &equation);
void displayCommonHelpMenu(std::string &equation);
void displayNumberTheoryHelpMenu(std::string &equation);
void displayStatisticsHelpMenu(std::string &equation);
void displayComplexHelpMenu(std::string &equation);
void displayMiscellaneousHelpMenu(std::string &equation);
void displayLogicHelpMenu(std::string &equation);
void displayCustomFunctionsHelpMenu(std::string &equation, std::vector<Instance> &instances, size_t &currInst);
void displayOperatorsHelpMenu(std::string &equation, std::unordered_map<std::string, lessetB::Operation> &operationContainer);
void displayHistory(std::string &resultHistory, lessetB::Options &options);
void displayScriptingHelpMenu();
void displayTechincalHelpMenu();

void toCopyButtonEachLine(std::string &str);
void toCopyButtonEachLine(std::string &str, size_t width);
[[nodiscard]] size_t calcMaxLineButtonWidth(std::string &str);
std::string getPriorityName(const lessetB::pass priority);
void resetGraphingData(std::vector<GraphEquation> &graphEquations, std::pair<std::vector<std::vector<double>>, std::vector<std::vector<double>>> &graphsPoints);
[[nodiscard]] std::string formatCoords(const GraphEquation &graphEquation, double x, double y, const lessetB::Options &options, bool withEquation=false);
void addEquation(std::vector<GraphEquation> &graphEquations, size_t &recalculateGraphIndex, std::pair<std::vector<std::vector<double>>,std::vector<std::vector<double>>> &graphsPoints);
void displayStyleMenu(ImGuiStyle &style, lessetB::Options &options, bool &gayMode, bool &showGayMode, float &gayModeSpeed, float &hueModifier, float &saturationModifier);

void graphsMenuTopBarUI(bool &showGraphingMenuHint, std::vector<GraphEquation> &graphEquations, size_t &recalculateGraphIndex,
               std::pair<std::vector<std::vector<double>>, std::vector<std::vector<double>>> &graphsPoints, lessetB::Options &options,
               std::vector<Instance> &instances, bool &recalculateGraphs, size_t &currInst);
#ifdef LESSET_INCLUDE_DEBUG
void debugMenu(std::vector<GraphEquation> &graphEquations, std::vector<Instance> &instances, size_t &currInst, const std::pair<std::vector<std::vector<double>>,std::vector<std::vector<double>>> &graphsPoints);
#endif
std::string colVecToHex(ImVec4 col);
bool isPartOfHexValue(const std::string &str);
bool convertHexToByte(const std::string &str, char8_t &byte);
[[nodiscard]] char convertToHexDigit(char8_t digit);
template<typename T>
requires std::is_arithmetic_v<T>
[[nodiscard]] ImVec4 ImVec4ScalarDiv(ImVec4 vec, T num);
template<typename T>
requires std::is_arithmetic_v<T>
[[nodiscard]] ImVec4 ImVec4ScalarMul(ImVec4 vec, T num);
template <typename T>
requires std::is_arithmetic_v<T>
[[nodiscard]] ImVec4 ImVec4ScalarAdd(ImVec4 vec, const T &scalar);
bool hasNAN(const std::pair<std::vector<std::vector<double>>, const std::vector<std::vector<double>>> &graphsPoints, const size_t j, const size_t increment, const size_t i);
ImPlotPoint markPoints(const std::vector<GraphEquation> &graphEquations, const size_t timeStationary,
               const std::pair<std::vector<std::vector<double>>, std::vector<std::vector<double>>> &graphsPoints, const lessetB::Options &options,
               const ImPlotRect &limits, const size_t eqIndex, ImPlotSpec &pointsSpec, bool &hasShownMousePointText, SpecialPoint &closestSpecialPointToCursor);
void setupPlot(const std::array<float, 2> &defaultBoundsX, const std::array<float, 2> &defaultBoundsY);
int inputFormatter(GraphEquation &graphEquation, const Instance &instance);
int inputFormatter(std::string &eq, const Instance &instance);

// -> Line 434




static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
static void check_vk_result(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

#ifdef APP_USE_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    (void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
    fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
    return VK_FALSE;
}
#endif // APP_USE_VULKAN_DEBUG_REPORT

static bool IsExtensionAvailable(const ImVector<VkExtensionProperties>& properties, const char* extension)
{
    for (const VkExtensionProperties& p : properties)
        if (strcmp(p.extensionName, extension) == 0)
            return true;
    return false;
}

static void SetupVulkan(ImVector<const char*> instance_extensions)
{
    VkResult err;
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
    volkInitialize();
#endif

    // Create Vulkan Instance
    {
        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        // Enumerate available extensions
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
        properties.resize(properties_count);
        err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.Data);
        check_vk_result(err);

        // Enable required extensions
        if (IsExtensionAvailable(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
            instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
        {
            instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
#endif

        // Enabling validation layers
#ifdef APP_USE_VULKAN_DEBUG_REPORT
        const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers;
        instance_extensions.push_back("VK_EXT_debug_report");
#endif

        // Create Vulkan Instance
        create_info.enabledExtensionCount = (uint32_t)instance_extensions.Size;
        create_info.ppEnabledExtensionNames = instance_extensions.Data;
        err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
        check_vk_result(err);
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
        volkLoadInstance(g_Instance);
#endif

        // Setup the debug report callback
#ifdef APP_USE_VULKAN_DEBUG_REPORT
        auto f_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkCreateDebugReportCallbackEXT");
        IM_ASSERT(f_vkCreateDebugReportCallbackEXT != nullptr);
        VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
        debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debug_report_ci.pfnCallback = debug_report;
        debug_report_ci.pUserData = nullptr;
        err = f_vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci, g_Allocator, &g_DebugReport);
        check_vk_result(err);
#endif
    }

    // Select Physical Device (GPU)
    g_PhysicalDevice = ImGui_ImplVulkanH_SelectPhysicalDevice(g_Instance);
    IM_ASSERT(g_PhysicalDevice != VK_NULL_HANDLE);

    // Select graphics queue family
    g_QueueFamily = ImGui_ImplVulkanH_SelectQueueFamilyIndex(g_PhysicalDevice);
    IM_ASSERT(g_QueueFamily != (uint32_t)-1);

    // Create Logical Device (with 1 queue)
    {
        ImVector<const char*> device_extensions;
        device_extensions.push_back("VK_KHR_swapchain");

        // Enumerate physical device extension
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &properties_count, nullptr);
        properties.resize(properties_count);
        vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr, &properties_count, properties.Data);
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
        if (IsExtensionAvailable(properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
            device_extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

        const float queue_priority[] = { 1.0f };
        VkDeviceQueueCreateInfo queue_info[1] = {};
        queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[0].queueFamilyIndex = g_QueueFamily;
        queue_info[0].queueCount = 1;
        queue_info[0].pQueuePriorities = queue_priority;
        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
        create_info.pQueueCreateInfos = queue_info;
        create_info.enabledExtensionCount = (uint32_t)device_extensions.Size;
        create_info.ppEnabledExtensionNames = device_extensions.Data;
        err = vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator, &g_Device);
        check_vk_result(err);
        vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
    }

    // Create Descriptor Pool
    // If you wish to load e.g. additional textures you may need to falter pools sizes and maxSets.
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE },
            { VK_DESCRIPTOR_TYPE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE },
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 0;
        for (VkDescriptorPoolSize& pool_size : pool_sizes)
            pool_info.maxSets += pool_size.descriptorCount;
        pool_info.poolSizeCount = (uint32_t)IM_COUNTOF(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        err = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
        check_vk_result(err);
    }
}

// All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers used by the demo.
// Your real engine/app may not use them.
static void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height)
{
    // Check for WSI support
    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily, surface, &res);
    if (res != VK_TRUE)
    {
        fprintf(stderr, "Error no WSI support on physical device 0\n");
        exit(-1);
    }

    // Select Surface Format
    const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    wd->Surface = surface;
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(g_PhysicalDevice, wd->Surface, requestSurfaceImageFormat, (size_t)IM_COUNTOF(requestSurfaceImageFormat), requestSurfaceColorSpace);

    // Select Present Mode
#ifdef APP_USE_UNLIMITED_FRAME_RATE
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
    VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(g_PhysicalDevice, wd->Surface, &present_modes[0], IM_COUNTOF(present_modes));
    //printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

    // Create SwapChain, RenderPass, Framebuffer, etc.
    IM_ASSERT(g_MinImageCount >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator, width, height, g_MinImageCount, 0);
}

static void CleanupVulkan()
{
    vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);

#ifdef APP_USE_VULKAN_DEBUG_REPORT
    // Remove the debug report callback
    auto f_vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkDestroyDebugReportCallbackEXT");
    f_vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
#endif // APP_USE_VULKAN_DEBUG_REPORT

    vkDestroyDevice(g_Device, g_Allocator);
    vkDestroyInstance(g_Instance, g_Allocator);
}

static void CleanupVulkanWindow(ImGui_ImplVulkanH_Window* wd)
{
    ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, wd, g_Allocator);
    vkDestroySurfaceKHR(g_Instance, wd->Surface, g_Allocator);
}

static void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data)
{
    VkSemaphore image_acquired_semaphore  = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkResult err = vkAcquireNextImageKHR(g_Device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        g_SwapChainRebuild = true;
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
        return;
    if (err != VK_SUBOPTIMAL_KHR)
        check_vk_result(err);

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
    {
        err = vkWaitForFences(g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
        check_vk_result(err);

        err = vkResetFences(g_Device, 1, &fd->Fence);
        check_vk_result(err);
    }
    {
        err = vkResetCommandPool(g_Device, fd->CommandPool, 0);
        check_vk_result(err);
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
        check_vk_result(err);
    }
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = wd->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = wd->Width;
        info.renderArea.extent.height = wd->Height;
        info.clearValueCount = 1;
        info.pClearValues = &wd->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    // Submit command buffer
    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        err = vkEndCommandBuffer(fd->CommandBuffer);
        check_vk_result(err);
        err = vkQueueSubmit(g_Queue, 1, &info, fd->Fence);
        check_vk_result(err);
    }
}

static void FramePresent(ImGui_ImplVulkanH_Window* wd)
{
    if (g_SwapChainRebuild)
        return;
    VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult err = vkQueuePresentKHR(g_Queue, &info);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        g_SwapChainRebuild = true;
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
        return;
    if (err != VK_SUBOPTIMAL_KHR)
        check_vk_result(err);
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount; // Now we can use the next set of semaphores
}



int g_maxIndividualGraphPointsMultiplier{2};
bool g_showAxes{true};
bool g_showLimits{true};
bool g_markSpecialPoints{true};
bool g_formatInput{true};


// Main code
int main(int argc, char **argv)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Create window with Vulkan context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    GLFWwindow* window = glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale), "Lesset", nullptr, nullptr);
    if (!glfwVulkanSupported())
    {
        printf("GLFW: Vulkan Not Supported\n");
        return 1;
    }

    ImVector<const char*> extensions;
    uint32_t extensions_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
    for (uint32_t i = 0; i < extensions_count; i++)
        extensions.push_back(glfw_extensions[i]);
    SetupVulkan(extensions);

    // Create Window Surface
    VkSurfaceKHR surface;
    VkResult err = glfwCreateWindowSurface(g_Instance, window, g_Allocator, &surface);
    check_vk_result(err);

    // Create Framebuffers
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
    SetupVulkanWindow(wd, surface, w, h);
    

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha=0.9f;
    style.FrameRounding=3.f;
    style.PopupRounding=3.f;
    style.WindowRounding=3.f;
    style.GrabRounding=3.f;

    // -> Line 477


    // Setup scaling
    // ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)
    io.ConfigDpiScaleFonts = true;          // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
    io.ConfigDpiScaleViewports = true;      // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    //init_info.ApiVersion = VK_API_VERSION_1_3;              // Pass in your value of VkApplicationInfo::apiVersion, otherwise will default to header version.
    init_info.Instance = g_Instance;
    init_info.PhysicalDevice = g_PhysicalDevice;
    init_info.Device = g_Device;
    init_info.QueueFamily = g_QueueFamily;
    init_info.Queue = g_Queue;
    init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.MinImageCount = g_MinImageCount;
    init_info.ImageCount = wd->ImageCount;
    init_info.Allocator = g_Allocator;
    init_info.PipelineInfoMain.RenderPass = wd->RenderPass;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Our state ☭
    auto& imGuiIO{ImGui::GetIO()};
    imGuiIO.IniFilename = NULL;
    imGuiIO.LogFilename = NULL;
    NFD::Init();

    ImVec4 clear_color = ImVec4(CLEAR_BRIGHTNESS,CLEAR_BRIGHTNESS,CLEAR_BRIGHTNESS, 1.f);
    io.Fonts->AddFontFromMemoryCompressedTTF(NotoSansMathRegular_compressed_data,NotoSansMathRegular_compressed_size);

    // Fix an issue with auto fitting
    constexpr std::array<float,2> defaultBoundsX{-10,10};
    constexpr std::array<float,2> defaultBoundsY{-10,10};

    // Display some text upon viewing a table calculation for the first time to use the graph menu for graphing, 
    // suspecting the user might've wanted to graph by typing into the regular calculation bar.
    bool showGraphingMenuHint{true}; 
    bool showTrivia{};


    // Gets cleared every frame
    std::string nothing;

    bool resultWindow{true};

    // Equation strings
    std::string lastNonEmptyEquation;
    std::vector<GraphEquation> graphEquations;
    std::string graphEquation;
    std::string nonEmptyGraphEquation;
    // Main entry field
    std::string equation;
    std::string nonEmptyEquation;
    std::string editingGraphEquation;
    std::string result;

    std::string resultHistory;

    // Graphing
    float pastPrecisionDivisors[100]{};
    size_t timeStationary{};
    size_t recalculateGraphIndex{static_cast<size_t>(-1)};
    int maxIndividualGraphPoints {MAX_GRAPH_POINTS_BASE}; // Maximum number of points per graph calculated for one screen. (There may be more because of memoization when zooming out) 

    std::pair<std::vector<std::vector<double>>,std::vector<std::vector<double>>> graphsPoints{}; // Big boy
    
    // Unsaved Settings / Runtime Settings
    lessetB::Options options{false,-5,5,boost::multiprecision::cpp_complex<lessetB::maxPrecision>(1,0),2,true,true,"",true,""};
    bool gayMode{};
    bool showGayMode{};
    float gayModeSpeed{1};

    bool logarithmicX{};
    bool logarithmicY{};
    
    bool interpolateDiscontinuities{};

    std::string aroundTruthinessLeniency="0.01";
    boost::multiprecision::cpp_complex<lessetB::maxPrecision> aroundTruthinessLeniencyNum{0.01};

    std::string xMin{"-5"};
    std::string xMax{'5'};
    std::string xStep={'1'};
    boost::multiprecision::cpp_complex<lessetB::maxPrecision> xMinFloat{-5};
    boost::multiprecision::cpp_complex<lessetB::maxPrecision> xMaxFloat{5};
    boost::multiprecision::cpp_complex<lessetB::maxPrecision> xStepFloat{0.1};

    bool drawMany_Graphs{true}; // cake_Case

    std::vector<Instance> instances{{"main"}};
    
    bool recalculateGraphs{};

    // Scripting
    size_t currInst{};
    
    
    float hueModifier{0.586};
    float saturationModifier{0.7};
    
    size_t previousGraphsEquationsSize{};
    int tooltipRnd{};
    int tooltipRnd2{};
    
    
    {
        ImGuiStyle initialStyle = style;
        ImGui::LoadStyleFrom("LessetStyle.ini");
        for(size_t i{}; i<ImGuiCol_COUNT; i++)
        {
            if(initialStyle.Colors[i].x!=style.Colors[i].x ||
            initialStyle.Colors[i].y!=style.Colors[i].y ||
            initialStyle.Colors[i].z!=style.Colors[i].z ||
            initialStyle.Colors[i].w!=style.Colors[i].w)
            {
                hueModifier=0.5;
                saturationModifier=0.5;
            }
        }
    }

    readSave(options);
    switch(g_maxIndividualGraphPointsMultiplier)
    {
        case 1: {maxIndividualGraphPoints=MAX_GRAPH_POINTS_BASE/2; break;}
        case 2: {maxIndividualGraphPoints=MAX_GRAPH_POINTS_BASE;  break;}
        case 3: {maxIndividualGraphPoints=MAX_GRAPH_POINTS_BASE*2; break;}
        case 4: {maxIndividualGraphPoints=MAX_GRAPH_POINTS_BASE*4; break;}
        case 5: {maxIndividualGraphPoints=MAX_GRAPH_POINTS_BASE/3; break;}

        default: {maxIndividualGraphPoints=MAX_GRAPH_POINTS_BASE*g_maxIndividualGraphPointsMultiplier; break;}
    }
    bool prioritizeImplicitMultiplication{options.prioritizeImplicitMultiplication};
    bool prettyPrinting{options.prettyPrinting};

    bool firstFrame{true};
    bool debugMode{};
    // -> Line 614

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Resize swap chain?
        int fb_width, fb_height;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        if (fb_width > 0 && fb_height > 0 && (g_SwapChainRebuild || g_MainWindowData.Width != fb_width || g_MainWindowData.Height != fb_height))
        {
            ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
            ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator, fb_width, fb_height, g_MinImageCount, 0);
            g_MainWindowData.FrameIndex = 0;
            g_SwapChainRebuild = false;
        }
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            #ifdef LESSET_INCLUDE_DEBUG
            lessetB::globals::debugCoutUsed=false;
            #endif
            nothing.clear();
            recalculateGraphIndex=static_cast<size_t>(-1);
            const auto &io = ImGui::GetIO();
            ImGui::SetNextWindowSize(io.DisplaySize);
            ImGui::SetNextWindowPos(ImVec2(0,0));
            style.WindowRounding=0.f;
            lessetB::globals::activeRecursions = 0;

            // Default first graph to button color every frame
            bool defaultFirstGraphColor=graphEquations.size()>0 && ImVec4Equal(graphEquations.at(0).col,style.Colors[ImGuiCol_Button],false);

            ImGui::Begin("Lesset says hello!",__null,ImGuiWindowFlags_NoScrollWithMouse|ImGuiWindowFlags_MenuBar|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoBringToFrontOnFocus|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoBackground);                          // Create a window called "Hello, world!" and append into it.
            style.WindowRounding=5.f;
            
            if (ImGui::BeginMenuBar())
            {
                // Scripting menu (These comments are to ctrl f faster lol)
                if(ImGui::BeginMenu("Scripting"))
                {
                    // Run Menu (Run Script Menu)
                    if (ImGui::BeginMenu("Run"))
                    {
                        static std::string filePath;
                        ImGui::Text("Path: ");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(fmin(fmax(ImGui::CalcTextSize(filePath.c_str()).x+20,48),io.DisplaySize.x/1.5));
                        ImGui::InputText("##",&filePath);

                        if(ImGui::Button("Run"))
                        {
                            runScript(graphEquations, options, instances, recalculateGraphs, currInst, filePath);
                            defaultFirstGraphColor=graphEquations.size()>0 && ImVec4Equal(graphEquations.at(0).col,style.Colors[ImGuiCol_Button],false);
                        }
                        ImGui::SetItemTooltip("Run the script specified in the path.");

                        ImGui::SameLine();
                        if(ImGui::Button("Search"))
                        {
                            nfdu8char_t *outPath{};
                            nfdopendialogu8args_t args = {0};
                            nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
                            if(result == NFD_OKAY)
                            {
                                filePath=outPath;
                                NFD_FreePathU8(outPath);
                            }
                        }
                        ImGui::SetItemTooltip("Open a file dialog.");

                        if(instances[currInst].hasRunScript)
                        {
                            ImGui::SameLine();
                            if(ImGui::Button("Clear data"))
                            {
                                instances[currInst]=Instance();
                            }
                            if(!instances[currInst].lastScriptOutput.empty())
                            {
                                std::string &lastScriptOutput = instances[currInst].lastScriptOutput;
                                if(ImGui::BeginMenu("Output"))
                                {
                                    std::string line;
                                    for(size_t i{}; i<lastScriptOutput.length();)
                                    {
                                        line=lastScriptOutput.substr(i,lastScriptOutput.find('\n',i)-i)+"##"+std::to_string(i);
                                        if(ImGui::Button(line.c_str()))
                                        {
                                            ImGui::SetClipboardText(lastScriptOutput.substr(i,lastScriptOutput.find('\n',i)-i).c_str());
                                        }
                                        ImGui::SetItemTooltip("Click to copy.");
                                        i+=line.length()-1-std::to_string(i).length();
                                    }
                                    // ImGui::Text("Output of script:\n%s", instances[currInst].lastScriptOutput.c_str());
                                    ImGui::EndMenu();
                                }
                            }
                        }

                        
                        
                        ImGui::EndMenu();
                    }
                    
                    ImGui::Separator();
                    if(ImGui::BeginMenu("Instances"))
                    {
                        static bool showError{};
                        if(ImGui::BeginMenu("Add"))
                        {
                            static std::string newInstanceName;
                            ImGui::SetNextItemWidth(fmax(ImGui::CalcTextSize(newInstanceName.c_str()).x+40,91));
                            ImGui::InputText("Name",&newInstanceName);
                            
                            if(ImGui::Button("Add Instance"))
                            {
                                bool nameOkay{};
                                if(!newInstanceName.empty()) nameOkay=true;
                                for(size_t i{}; i<instances.size(); i++)
                                {
                                    if(newInstanceName==instances.at(i).name || newInstanceName.length()>MAX_KEYWORD_LENGTH) nameOkay=false;
                                }
                                if(nameOkay) instances.emplace_back(newInstanceName);
                                else showError=true;
                            }  
                            if(showError)
                            {
                                ImGui::Text("Cannot use this name.");
                            }
                            ImGui::EndMenu(); 
                        }   
                        else showError=false;
                        size_t width{};  
                        for(auto instance : instances)
                        {
                            width=fmax(width,ImGui::CalcTextSize(instance.name.c_str()).x+20);
                        }
                        if(instances.size()>1)
                            if(ImGui::BeginMenu("Select"))
                            {
                                for(size_t i{}; i<instances.size(); i++)
                                {
                                    if(ImGui::Button(instances.at(i).name.c_str(),ImVec2(width,ROW_HEIGHT)))
                                    {
                                        currInst=i;
                                        recalculateGraphs=true;
                                    }
                                }    
                                ImGui::EndMenu();
                            }

                        if(instances.size()>1) if(ImGui::BeginMenu("Remove"))
                        {
                            size_t width{};  
                            for(Instance instance : instances)
                            {
                                if(instance.name!="main")
                                    width=fmax(width,ImGui::CalcTextSize(instance.name.c_str()).x+20);
                            }
                            for(size_t i{1}; i<instances.size(); i++)
                            {
                                if(ImGui::Button(instances.at(i).name.c_str(),ImVec2(width,ROW_HEIGHT)))
                                {
                                    if(currInst==i) currInst=MAIN_INSTANCE;
                                    instances.erase(instances.begin()+i--);
                                }
                            }  
                            ImGui::EndMenu();
                        }

                        ImGui::EndMenu();
                    }
                    ImGui::SetItemTooltip("Encapsulate calculator data.");
                    // Variables Menu
                    if(ImGui::BeginMenu("Variables"))
                    {
                        std::string label="Let";

                        std::uniform_int_distribution<> intDist(0,1000);
                        if(tooltipRnd2==0) tooltipRnd2 = intDist(lessetB::randomMt);
                        if (tooltipRnd2!=1000) label="Let";
                        else label="Lesset";

                        if (ImGui::BeginMenu(label.c_str()))
                        {
                            static std::string newVarName;
                            static std::string newVarValue;
                            static std::string report;

                            ImGui::SetNextItemWidth(fmax(fmin(fmax(ImGui::CalcTextSize(newVarName.c_str()).x + 20, 48), io.DisplaySize.x / 1.5),
                                                         fmin(fmax(ImGui::CalcTextSize(newVarValue.c_str()).x + 20, 48), io.DisplaySize.x / 1.5)));
                            ImGui::InputText("Name", &newVarName);
                            if(newVarName.length() > MAX_KEYWORD_LENGTH) newVarName = newVarName.substr(0, MAX_KEYWORD_LENGTH);
                            ImGui::SetNextItemWidth(fmax(fmin(fmax(ImGui::CalcTextSize(newVarName.c_str()).x + 20, 48), io.DisplaySize.x / 1.5),
                                                         fmin(fmax(ImGui::CalcTextSize(newVarValue.c_str()).x + 20, 48), io.DisplaySize.x / 1.5)));
                            ImGui::InputText("Value", &newVarValue);
                            if(g_formatInput) inputFormatter(newVarValue,instances[currInst]);
                            if(ImGui::Button("Assign"))
                            {

                                if(addVariable(std::pair<std::string,std::string>(newVarName,newVarValue),instances[currInst])) 
                                    report="Cannot use this name.";
                                
                                else 
                                {
                                    recalculateGraphs=true;
                                    boost::multiprecision::cpp_complex<lessetB::maxPrecision> value =
                                        static_cast<boost::multiprecision::cpp_complex<lessetB::maxPrecision>>(
                                            instances[currInst].customSymbols.m_variables.find(newVarName)->second);
                                    report="Assigned \"" + newVarName + "\" value " + value.str(9);
                                }
                            }
                            if(!report.empty()) ImGui::Text("%s",report.c_str());

                            ImGui::EndMenu();
                        }
                        if (instances[currInst].customSymbols.m_variables.size()!=0 && ImGui::BeginMenu("Show"))
                        {
                            if(instances[currInst].customSymbols.m_variables.size()==0) ImGui::MenuItem("You have no variables.",NULL,false,false);
                            else 
                            {
                                ImGui::MenuItem("Click a variable to delete it.",NULL,false,false);
                                ImGui::Separator();
                            }
                            std::string formatted;
                            size_t i{};
                            for(auto var : instances[currInst].customSymbols.m_variables)
                            {
                                instances[currInst].sliders.emplace(
                                    std::pair<std::string,
                                              SliderData>{var.first,
                                                          SliderData{
                                                              {static_cast<float>(
                                                                   static_cast<boost::multiprecision::cpp_complex<lessetB::maxPrecision>>(var.second)
                                                                       .real()),
                                                               0, 10}}});
                                formatted=var.first+" = ";
                                std::ostringstream oss;
                                oss<<std::fixed;
                                oss<<std::setprecision(3);
                                
                                oss<<static_cast<boost::multiprecision::cpp_complex<lessetB::maxPrecision>>(var.second).real();
                                if(oss.str().length()>20)
                                {
                                    oss.str("");
                                    oss<<std::scientific;
                                    oss.precision(4);
                                    oss<<static_cast<boost::multiprecision::cpp_complex<lessetB::maxPrecision>>(var.second).real();
                                }
                                formatted+=oss.str();
                                ImGui::SetNextItemWidth(110);
                                if(ImGui::Button(formatted.c_str()))
                                {
                                    instances[currInst].customSymbols.m_variables.erase(var.first);
                                    instances[currInst].sliders.erase(var.first);
                                    recalculateGraphs=true;
                                    break;
                                }

                                ImGui::SameLine();
                                if(ImGui::BeginMenu(std::string("Settings##"+std::to_string(i)).c_str()))
                                {
                                    if(instances[currInst].sliders.at(var.first).nums[3]==0) instances[currInst].sliders.at(var.first).nums[3]=1;
                                    ImGui::Checkbox("Animate", &instances[currInst].sliders.at(var.first).animate);
                                    ImGui::SetItemTooltip("This will cause all graphs to be recalculated every frame. Don't fry your computer lol");
                                    ImGui::SameLine();
                                    static size_t prevI{};
                                    static std::string btnLabel=" ← → ";
                                    if(ImGui::Button(btnLabel.c_str()))
                                    {
                                        using enum sliderAnimMode;
                                        if(instances[currInst].sliders.at(var.first).animMode==TO_FRO)
                                        {
                                            instances[currInst].sliders.at(var.first).animMode=TO;
                                            btnLabel="  →  ";
                                        }
                                        else if(instances[currInst].sliders.at(var.first).animMode==TO)
                                        {
                                            instances[currInst].sliders.at(var.first).animMode=TO_FRO_SMOOTH;
                                            btnLabel=" ←s→ ";
                                        }
                                        else if(instances[currInst].sliders.at(var.first).animMode==TO_FRO_SMOOTH)
                                        {
                                            instances[currInst].sliders.at(var.first).animMode=TO_FRO;
                                            btnLabel=" ← → ";
                                        }
                                    }
                                    if(prevI!=i)
                                    {
                                        using enum sliderAnimMode;
                                        if(instances[currInst].sliders.at(var.first).animMode==TO_FRO)
                                        {
                                            btnLabel=" ← → ";
                                        }
                                        else if(instances[currInst].sliders.at(var.first).animMode==TO)
                                        {
                                            btnLabel="  →  ";
                                        }
                                        else if(instances[currInst].sliders.at(var.first).animMode==TO_FRO_SMOOTH)
                                        {
                                            btnLabel=" ←s→ ";
                                        }
                                    }
                                    ImGui::SetNextItemWidth(200);
                                    ImGui::SliderFloat("##sl", instances[currInst].sliders.at(var.first).nums.data() + 3, 0.001, 10,
                                                       "Animation speed: %.3f", ImGuiSliderFlags_Logarithmic);
                                    if(instances[currInst].sliders.at(var.first).nums[3]<FLT_MIN) instances[currInst].sliders.at(var.first).nums[3]=FLT_MIN;
                                    else if(instances[currInst].sliders.at(var.first).nums[3]>1000) instances[currInst].sliders.at(var.first).nums[3]=1000;
                                    ImGui::EndMenu();
                                    prevI=i;
                                }
                                ImGui::SetNextItemWidth(146);
                                ImGui::DragFloat(std::string("##min" + std::to_string(i)).c_str(),
                                                 instances[currInst].sliders.at(var.first).nums.data() + 1, 0.2, -999999, 999999, "Min: %.3f");
                                ImGui::SetNextItemWidth(146);
                                ImGui::SameLine();
                                ImGui::DragFloat(std::string("##max" + std::to_string(i)).c_str(),
                                                 instances[currInst].sliders.at(var.first).nums.data() + 2, 0.2, -999999, 999999, "Max: %.3f");

                                if(instances[currInst].sliders.at(var.first).nums[2] != instances[currInst].sliders.at(var.first).nums[2])
                                    instances[currInst].sliders.at(var.first).nums[2] = 0;
                                if(instances[currInst].sliders.at(var.first).nums[1] != instances[currInst].sliders.at(var.first).nums[1])
                                    instances[currInst].sliders.at(var.first).nums[1] = 0;

                                if(instances[currInst].sliders.at(var.first).nums[2]>FLT_MAX/2) instances[currInst].sliders.at(var.first).nums[2]=FLT_MAX/2;
                                if(instances[currInst].sliders.at(var.first).nums[2]<-FLT_MAX/2) instances[currInst].sliders.at(var.first).nums[2]=-FLT_MAX/2;

                                if(instances[currInst].sliders.at(var.first).nums[1]>FLT_MAX/2) instances[currInst].sliders.at(var.first).nums[1]=FLT_MAX/2;
                                if(instances[currInst].sliders.at(var.first).nums[1]<-FLT_MAX/2) instances[currInst].sliders.at(var.first).nums[1]=-FLT_MAX/2;
                                
                                ImGui::SetNextItemWidth(300);
                                if(ImGui::SliderFloat(std::string("##"+std::to_string(i)).c_str(),instances[currInst].sliders.at(var.first).nums.data(),instances[currInst].sliders.at(var.first).nums[1],instances[currInst].sliders.at(var.first).nums[2],"Slider"))
                                {
                                    addVariable(std::pair<std::string,std::string>{var.first,std::to_string(instances[currInst].sliders.at(var.first).nums[0])}, instances[currInst]);
                                    recalculateGraphs=true;
                                }
                                if(i+1<instances[currInst].customSymbols.m_variables.size()) ImGui::Separator();
                                i++;
                            }
                            ImGui::EndMenu();
                        }
                        ImGui::EndMenu();
                    }
                    else tooltipRnd2=0;
                    if(ImGui::BeginMenu("Operators"))
                    {
                        if(ImGui::BeginMenu("Set"))
                        {
                            static std::string newIdentifierNameOperator="name(param1,param2)";
                            static std::string newIdentifierValueOperator;
                            if(g_formatInput)
                            {
                                
                                addClosingParentheses(newIdentifierNameOperator);
                                if(g_formatInput) inputFormatter(newIdentifierValueOperator,instances[currInst]);
                            }
                            static std::string assignOperatorReport;

                            static bool rightAssociative{};
                            static bool postfix{};
                            static int priority{lessetB::MULTIPLICATION};

                            static std::string priorityFmt;

                            priorityFmt=getPriorityName(static_cast<lessetB::pass>(priority));

                            
                            ImGui::SetNextItemWidth(fmax(fmin(fmax(ImGui::CalcTextSize(newIdentifierNameOperator.c_str()).x+20,148),io.DisplaySize.x/1.5),fmin(fmax(ImGui::CalcTextSize(newIdentifierValueOperator.c_str()).x+20,48),io.DisplaySize.x/1.5)));
                            ImGui::InputText("Signature",&newIdentifierNameOperator);
                            ImGui::SetItemTooltip("Specifying one argument makes a unary operator, two a binary one.\ni.e.: name(b,d) for a binary operator.");
                            ImGui::SetNextItemWidth(fmax(fmin(fmax(ImGui::CalcTextSize(newIdentifierNameOperator.c_str()).x+20,148),io.DisplaySize.x/1.5),fmin(fmax(ImGui::CalcTextSize(newIdentifierValueOperator.c_str()).x+20,48),io.DisplaySize.x/1.5)));
                            ImGui::InputText("Expression",&newIdentifierValueOperator);
                            ImGui::Checkbox("Is right-associative", &rightAssociative);
                            ImGui::SameLine();
                            ImGui::Checkbox("Postfix", &postfix);
                            ImGui::SetItemTooltip("This only applies to unary operators.");
                            ImGui::SetNextItemWidth(182);
                            ImGui::SliderInt("##", &priority,lessetB::CS1,lessetB::CS18, priorityFmt.c_str(),ImGuiSliderFlags_AlwaysClamp|ImGuiSliderFlags_NoInput);
                            if(static_cast<lessetB::pass>(priority)!=lessetB::MULTIPLICATION_IMPLICIT) ImGui::SetItemTooltip("The precedence of your operator.");
                            else ImGui::SetItemTooltip("This pass is still active when Prioritize Implicit Multiplication is unchecked.");
                            ImGui::SameLine();
                            if(ImGui::Button("Assign"))
                            {
                                if(!newIdentifierNameOperator.empty() && !newIdentifierValueOperator.empty())
                                {
                                    recalculateGraphs=true;
                                    symbolCreationError err = getOperationFromArgs(newIdentifierNameOperator,newIdentifierValueOperator,static_cast<lessetB::pass>(priority),lessetB::operatorAttributes(1<<(3+rightAssociative)|1<<(1+postfix)),instances[currInst]);
                                    if(std::to_underlying(err))
                                    {
                                        using enum symbolCreationError;
                                        switch(err)
                                        {
                                            case BAD_NAME: assignOperatorReport="Bad operator name."; break;
                                            case BAD_PARAMS: assignOperatorReport="Bad parameter name(s)."; break;
                                            case BAD_DEFINITION: assignOperatorReport="Bad definition."; break;
                                            case DOUBLED_CUSTOM_NAME: assignOperatorReport="Cannot reuse variable name."; break;
                                            case BAD_SIGNATURE: assignOperatorReport="Bad signature."; break;
                                            default: assignOperatorReport="what"; break;
                                        }
                                    }
                                    else 
                                    {
                                        auto &op = instances[currInst].customSymbols.m_operations[newIdentifierNameOperator.substr(0,newIdentifierNameOperator.find('('))];
                                        using enum lessetB::operatorAttributes;
                                        if(op.m_info==(U_PREFIX|R_ASSOCIATIVE) && op.m_precedence==lessetB::pass::FUNCTIONS)
                                            assignOperatorReport="Single-argument function \"" + newIdentifierNameOperator + "\" = " + newIdentifierValueOperator;
                                        else assignOperatorReport="Assigned \"" + newIdentifierNameOperator + "\" value " + newIdentifierValueOperator;
                                    }
                                }
                                else assignOperatorReport="Please define both fields first.";
                            }

                            if(!assignOperatorReport.empty())
                            {
                                static std::string tooltip;
                                if(newIdentifierNameOperator.find('(')!=std::string::npos)
                                    tooltip=std::format("You've defined a single argument function.\nThis means writing something like {0} 3x will behave in the same way as {0}(3x).", newIdentifierNameOperator.substr(0,newIdentifierNameOperator.find('(')).c_str());
                                
                                ImGui::Text("%s",assignOperatorReport.c_str());
                                if(assignOperatorReport[0]=='S')
                                {
                                    ImGui::SetItemTooltip("%s",tooltip.c_str());
                                }
                            }

                            ImGui::EndMenu();
                        }
                        auto &ops = instances[currInst].customSymbols.m_operations;
                        if(ops.size()>0 && ImGui::BeginMenu("Erase"))
                        {
                            for(auto op : ops)
                            {
                                if(ImGui::MenuItem(op.first.c_str()))
                                {
                                    ops.erase(op.first);
                                    break;
                                }
                                ImGui::SetItemTooltip("%s",op.second.m_def.c_str());
                            }
                            ImGui::EndMenu();
                        }
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }
                ImGui::SetItemTooltip("Automate certain things.");

                // Constants Menu
                if (ImGui::BeginMenu("Constants"))
                {
                    if(ImGui::BeginMenu("Mathematics"))
                    {
                        displayMathConstantsMenu(equation);
                    }

                    if(ImGui::BeginMenu("Physics"))
                    {
                        displayPhysicsConstantsMenu(equation);
                    }
                    
                    ImGui::EndMenu();
                }
                ImGui::SetItemTooltip("Add certain constants to your equation.");

                // Graph Menu
                if(ImGui::BeginMenu("Graph"))
                {
                    graphsMenuTopBarUI(showGraphingMenuHint, graphEquations, recalculateGraphIndex, graphsPoints, options, instances, recalculateGraphs,currInst);

                    for(size_t i{}; i<graphEquations.size(); i++)
                    {                        
                        std::string fnSig=graphEquations.at(i).fn.first+"(";
                        for(size_t j{}; j<graphEquations.at(i).fn.second.argNames.size(); j++)
                        {
                            std::string argName = graphEquations.at(i).fn.second.argNames.at(j);
                            fnSig+=argName;
                            if(j+1<graphEquations.at(i).fn.second.argNames.size()) fnSig+=",";
                        }
                        fnSig+=")";
                        if(fnSig=="()") fnSig.clear();
                        editingGraphEquation=graphEquations.at(i).eq;
                        if(i>0)
                        {
                            if(ImGui::Button(std::string("↑##" + std::to_string(i)).c_str(),ImVec2(ROW_HEIGHT,ROW_HEIGHT))) 
                            {
                                std::swap(graphEquations.at(i),graphEquations.at(i-1));
                                std::swap(graphsPoints.first.at(i),graphsPoints.first.at(i-1));
                                std::swap(graphsPoints.second.at(i),graphsPoints.second.at(i-1));
                                defaultFirstGraphColor=graphEquations.size()>0 && ImVec4Equal(graphEquations.at(0).col,style.Colors[ImGuiCol_Button],false);
                            }
                            ImGui::SetItemTooltip("Move this item up.");
                            ImGui::SameLine();
                        }
                        else if(graphEquations.size()>1)
                        {
                            if(ImGui::Button("↓",ImVec2(ROW_HEIGHT,ROW_HEIGHT))) 
                            {
                                std::swap(graphEquations.at(i+1),graphEquations.at(i));
                                std::swap(graphsPoints.first.at(i),graphsPoints.first.at(i+1));
                                std::swap(graphsPoints.second.at(i),graphsPoints.second.at(i+1));
                                defaultFirstGraphColor=graphEquations.size()>0 && ImVec4Equal(graphEquations.at(0).col,style.Colors[ImGuiCol_Button],false);
                            }
                            ImGui::SetItemTooltip("Move this item down.");
                            ImGui::SameLine();
                        }
                        if(graphEquations.size()>10)
                        {
                            if(i!=0)
                            {
                                ImGui::SameLine();
                                if(ImGui::Button(std::string("↟##" + std::to_string(i)).c_str(),ImVec2(ROW_HEIGHT,ROW_HEIGHT))) 
                                {
                                    std::swap(graphEquations.at(i),graphEquations.at(0));
                                    std::swap(graphsPoints.first.at(i),graphsPoints.first.at(0));
                                    std::swap(graphsPoints.second.at(i),graphsPoints.second.at(0));
                                    defaultFirstGraphColor=graphEquations.size()>0 && ImVec4Equal(graphEquations.at(0).col,style.Colors[ImGuiCol_Button],false);
                                }
                                ImGui::SetItemTooltip("Swap this item with the top.");
                                ImGui::SameLine();
                            }
                            else
                            {
                                ImGui::SameLine();
                                if(ImGui::Button(std::string("↡##" + std::to_string(i)).c_str(),ImVec2(ROW_HEIGHT,ROW_HEIGHT))) 
                                {
                                    std::swap(graphEquations.at(i),graphEquations.at(graphEquations.size()-1));
                                    std::swap(graphsPoints.first.at(i),graphsPoints.first.at(graphsPoints.first.size()-1));
                                    std::swap(graphsPoints.second.at(i),graphsPoints.second.at(graphsPoints.second.size()-1));
                                    defaultFirstGraphColor=graphEquations.size()>0 && ImVec4Equal(graphEquations.at(0).col,style.Colors[ImGuiCol_Button],false);
                                }
                                ImGui::SetItemTooltip("Swap this item with the bottom.");
                                ImGui::SameLine();
                            }
                        }
                        
                        ImGui::ColorEdit4(std::string("##col"+std::to_string(i)).c_str(), &graphEquations.at(i).col.x,ImGuiColorEditFlags_NoInputs);
                        if(i==0) defaultFirstGraphColor=graphEquations.size()>0 && ImVec4Equal(graphEquations.at(0).col,style.Colors[ImGuiCol_Button],false);
                        ImGui::SameLine();
                        if(ImGui::InputText(std::string("##" + std::to_string(i)).c_str(), &editingGraphEquation))
                        {
                            if(i+1==graphEquations.size())
                            {
                                graphEquations.emplace_back();
                                recalculateGraphIndex=graphEquations.size()-1;
                                graphsPoints.first.emplace_back(std::vector<double>({NAN}));
                                graphsPoints.second.emplace_back(std::vector<double>({NAN}));
                            }
                            std::string editingGraphEquationFormatted=editingGraphEquation;

                            if(g_formatInput) inputFormatter(editingGraphEquationFormatted,instances[currInst]);
                            

                            if(!(graphEquations.at(i).eq==editingGraphEquationFormatted || editingGraphEquationFormatted.length()>1000))
                            {
                                std::string fnName;
                                if(fnSig.find('(')!=std::string::npos)
                                {
                                    fnName=fnSig.substr(0,fnSig.find('('));
                                }
                                if(fnName!="")
                                {                            
                                    if(lessetB::globals::multiArgFunctions.find(fnName)==lessetB::globals::multiArgFunctions.end() &&
                                    (lessetB::globals::symbols.find(fnName)==lessetB::globals::symbols.end() || lessetB::globals::symbols.find(fnName)->second!=lessetB::tokenType::FUNCTION) )
                                    {
        
                                        if(instances[currInst].customSymbols.m_functions.find(fnName)!=instances[currInst].customSymbols.m_functions.end())
                                        {
                                            if(instances[currInst].customSymbols.m_functions.find(fnName)->second.definition!=editingGraphEquation)
                                            {
                                                instances[currInst].customSymbols.m_functions.at(fnName).definition=editingGraphEquation;
                                            }
                                        }
                                        else
                                        {
                                            instances[currInst].customSymbols.m_functions.emplace(fnName,editingGraphEquation);
                                        }
                                        graphEquations.at(i).fn=std::pair<std::string,lessetB::Function>(fnName,getFnFromArgs(graphEquations.at(i).eq,fnSig,instances[currInst]));
                                    }
                                    recalculateGraphs=true;
                                }

                                graphEquations.at(i).eq=editingGraphEquationFormatted; 
                                graphEquations.at(i).setTimePerPoint(instances[currInst]);
                                recalculateGraphIndex=i;
                                
                            }
                        }
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(80);
                        if(ImGui::InputText(std::string("##fn" + std::to_string(i)).c_str(), &fnSig))
                        {
                            if(fnSig.find(')')==std::string::npos) fnSig.push_back(')');
                            std::string fnName;
                            if(fnSig.find('(')!=std::string::npos)
                            {
                                fnName=fnSig.substr(0,fnSig.find('('));
                            }
                            lessetB::isValidName(fnName, true);
                            // std::cout<<fnName<<'\n';
                            if(fnName!="" && fnName.find('x')==std::string::npos)
                            {                            
                                if(instances[currInst].protectedFunctionDefinitions.find(fnName)==instances[currInst].protectedFunctionDefinitions.end() &&
                                lessetB::globals::multiArgFunctions.find(fnName)==lessetB::globals::multiArgFunctions.end() &&
                                (lessetB::globals::symbols.find(fnName)==lessetB::globals::symbols.end() || lessetB::globals::symbols.find(fnName)->second!=lessetB::tokenType::FUNCTION) )
                                {
                                    graphEquations.at(i).fn=std::pair<std::string,lessetB::Function>(fnName,getFnFromArgs(graphEquations.at(i).eq,fnSig,instances[currInst]));
                                    if(instances[currInst].customSymbols.m_functions.find(fnName)!=instances[currInst].customSymbols.m_functions.end())
                                    {
                                        if(instances[currInst].customSymbols.m_functions.find(fnName)->second.definition!=graphEquations.at(i).fn.second.definition ||
                                        instances[currInst].customSymbols.m_functions.find(fnName)->second.argNames!=graphEquations.at(i).fn.second.argNames)
                                        {
                                            instances[currInst].customSymbols.m_functions.at(fnName)=graphEquations.at(i).fn.second;
                                        }
                                    }
                                    else instances[currInst].customSymbols.m_functions.emplace(fnName,getFnFromArgs(graphEquations.at(i).eq,fnSig,instances[currInst]));
                                }
                                else ImGui::SetItemTooltip("Cannot use this name.");
                                
                            }
                        }
                        ImGui::SameLine();
                        if(ImGui::Button(std::string("×##" + std::to_string(i)).c_str(),ImVec2(ROW_HEIGHT,ROW_HEIGHT)))
                        {
                            if(instances[currInst].customSymbols.m_functions.find(graphEquations.at(i).fn.first)!=instances[currInst].customSymbols.m_functions.end())
                            {
                                instances[currInst].customSymbols.m_functions.erase(graphEquations.at(i).fn.first);
                                lessetB::globals::tokenMemory.clear();
                            }

                            graphEquations.erase(graphEquations.begin()+i);
                            if(i==0) defaultFirstGraphColor=graphEquations.size()>0 && ImVec4Equal(graphEquations.at(0).col,style.Colors[ImGuiCol_Button],false);
                            if(graphsPoints.first.size()!=0)
                            {
                                graphsPoints.first.erase(graphsPoints.first.begin()+i);
                                graphsPoints.second.erase(graphsPoints.second.begin()+i);
                                graphsPoints.first.shrink_to_fit();
                                graphsPoints.second.shrink_to_fit();
                                recalculateGraphs=true;
                            }
                            if(graphEquations.size()==0) // Prevent one frame from looking funny
                            {
                                graphEquations.emplace(graphEquations.begin());
                                graphEquations.at(0).col=ImGui::GetStyle().Colors[ImGuiCol_Button];
                                graphEquations.at(0).col.w+=0.4;
                            }
                        }
                        if(editingGraphEquation.length()>1000)
                        {
                            ImGui::Text("Equation should not be longer than 1000 characters.\nPlease, what are you doing?");
                        }
                    
                    }

                    // Erase functions which's definition has been removed or if it was renamed, unless defined in a script
                    std::unordered_map<std::string, char> names;
                    for(size_t i{}; i<graphEquations.size(); i++)
                    {
                        names.emplace(graphEquations.at(i).fn.first,'c');
                    }
                    names.insert_range(instances[currInst].protectedFunctionDefinitions);
                    for(auto fn : instances[currInst].customSymbols.m_functions)
                    {
                        if(names.find(fn.first)==names.end())
                        {
                            instances[currInst].customSymbols.m_functions.erase(fn.first);
                            break;
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::SetItemTooltip("Graph functions.");

                // Options Menu
                if(ImGui::BeginMenu("Options"))
                {

                    if(ImGui::BeginMenu("Tables"))
                    {

                        {
                            static size_t width{20};
                            width=fmin(width,io.DisplaySize.x/1.5);
                            width=fmax(width,150);
                            
                            ImGui::SetNextItemWidth(width);
                            if(ImGui::InputText("Min", &xMin,ImGuiInputTextFlags_CharsScientific))
                            {
                                lessetB::Options evalOptions{false,
                                                                0,
                                                                0,
                                                                0,
                                                                0,
                                                                false,
                                                                0,
                                                                "",
                                                                0,
                                                                ""};
                                std::string tmp=xMin;
                                if(g_formatInput) inputFormatter(xMin,instances[currInst]);
                                width=ImGui::CalcTextSize(xMin.c_str()).x+40;
                                lessetB::evaluateEquation(evalOptions, true, false, tmp, nothing, xMin, 
                                                          instances[currInst].customSymbols.m_variables,
                                                          instances[currInst].customSymbols.m_functions,
                                                          instances[currInst].customSymbols.m_operations);
                            }
                            else width=ImGui::CalcTextSize(xMin.c_str()).x+40;
                        }


                        if(lessetB::isRealNumber(xMin) && lessetB::isRealNumber(xMax) && xMin!="" && xMax!="")
                        {
                            if(options.xMax.real()<options.xMin.real()) ImGui::SetItemTooltip("Max is currently less than Min.");
                        }

                        if(lessetB::isRealNumber(xMin) && xMin!="" && xMin!="-")
                        {
                            options.xMin=static_cast<boost::multiprecision::cpp_complex<lessetB::maxPrecision>>(xMin);
                        }
                        
                        { 
                            static size_t width{20};
                            width=fmin(width,io.DisplaySize.x/1.5);
                            width=fmax(width,150);
                            
                            ImGui::SetNextItemWidth(width);
                            if(ImGui::InputText("Max", &xMax,ImGuiInputTextFlags_CharsScientific))
                            {
                                lessetB::Options evalOptions{false,
                                                                0,
                                                                0,
                                                                0,
                                                                0,
                                                                false,
                                                                0,
                                                                "",
                                                                0,
                                                                ""};
                                std::string tmp=xMax;
                                if(g_formatInput) inputFormatter(xMax,instances[currInst]);
                                width=ImGui::CalcTextSize(xMax.c_str()).x+40;
                                lessetB::evaluateEquation(evalOptions, true, false, tmp, nothing, xMax, 
                                                          instances[currInst].customSymbols.m_variables,
                                                          instances[currInst].customSymbols.m_functions,
                                                          instances[currInst].customSymbols.m_operations);
                            }
                            else width=ImGui::CalcTextSize(xMax.c_str()).x+40;
                        }
                        

                        if(lessetB::isRealNumber(xMin) && lessetB::isRealNumber(xMax) && xMin!="" && xMax!="")
                        {
                            if(options.xMax.real()<options.xMin.real()) ImGui::SetItemTooltip("Max is currently less than Min.");
                        }

                        if(lessetB::isRealNumber(xMax) && xMax!="" && xMax!="-")
                        {
                            options.xMax=static_cast<boost::multiprecision::cpp_complex<lessetB::maxPrecision>>(xMax);
                        }

                        std::string lastXStep=xStep;
                        {
                            static size_t width{20};
                            width=fmin(width,io.DisplaySize.x/1.5);
                            width=fmax(width,150);
                            ImGui::SetNextItemWidth(width);
                            
                            if(ImGui::InputText("Step", &xStep,ImGuiInputTextFlags_CharsScientific))
                            {
                                std::string tmp=xStep;
                                lessetB::Options evalOptions{false,
                                                                0,
                                                                0,
                                                                0,
                                                                0,
                                                                false,
                                                                0,
                                                                "",
                                                                0,
                                                                ""};
                                if(g_formatInput) inputFormatter(xStep,instances[currInst]);
                                width=ImGui::CalcTextSize(xStep.c_str()).x+40;

                                lessetB::evaluateEquation(evalOptions, true, false, tmp, nothing, xStep,
                                                          instances[currInst].customSymbols.m_variables,
                                                          instances[currInst].customSymbols.m_functions,
                                                          instances[currInst].customSymbols.m_operations);
                            }
                            else width=ImGui::CalcTextSize(xStep.c_str()).x+40;
                        }
                        

                        if(lessetB::isRealNumber(xStep) && xStep!="" && static_cast<boost::multiprecision::cpp_complex<lessetB::maxPrecision>>(xStep).real()>0)
                        {
                            options.xStep=static_cast<boost::multiprecision::cpp_complex<lessetB::maxPrecision>>(xStep);
                        }
                        else xStep=lastXStep;
                        if(static_cast<double>(abs(options.xMax-options.xMin)/abs(options.xStep))+1>100000) ImGui::Text("Maximum points for calculation is 100,000"); // It's 100,001 but who cares
                        else if(static_cast<double>(abs(options.xMax-options.xMin)/abs(options.xStep))+1>1000) ImGui::SetItemTooltip("You'll calculate over 1,000 points.");

                        ImGui::EndMenu();
                    }
                    ImGui::SetItemTooltip("Adjust bounds and step size for table calculations.");

                    if(ImGui::BeginMenu("Graphs"))
                    {
                        if(!debugMode)
                        {
                            if(g_maxIndividualGraphPointsMultiplier>5) g_maxIndividualGraphPointsMultiplier=5;
                            if(g_maxIndividualGraphPointsMultiplier<1) g_maxIndividualGraphPointsMultiplier=1;
                        }
                        std::string shownResolution="Medium";
                        
                        switch(g_maxIndividualGraphPointsMultiplier)
                        {
                            case 1: {maxIndividualGraphPoints=MAX_GRAPH_POINTS_BASE/2; shownResolution="Low"; break;}
                            case 2: {maxIndividualGraphPoints=MAX_GRAPH_POINTS_BASE; shownResolution="Medium"; break;}
                            case 3: {maxIndividualGraphPoints=MAX_GRAPH_POINTS_BASE*2; shownResolution="High"; break;}
                            case 4: {maxIndividualGraphPoints=MAX_GRAPH_POINTS_BASE*4; shownResolution="Overkill"; break;}
                            case 5: {maxIndividualGraphPoints=MAX_GRAPH_POINTS_BASE/3; shownResolution="Too Low"; break;}

                            default: {maxIndividualGraphPoints=MAX_GRAPH_POINTS_BASE*g_maxIndividualGraphPointsMultiplier; shownResolution="What"; break;}
                        }
                        ImGui::SetNextItemWidth(149.f);
                        if(ImGui::SliderInt("##",&g_maxIndividualGraphPointsMultiplier,1,5,std::string("Detail: "+shownResolution).c_str(),ImGuiSliderFlags_NoInput))
                        {
                            writeSave(options);
                            recalculateGraphs=true;
                        }
                        ImGui::SetItemTooltip(
                            "Changes how precisely graphs are calculated, how many points per graph.\nOverkill might be useful for detailed "
                            "graphs.\nOnly use Too Low if you have many graphs, are creating insane functions or your computer sux.");

                        ImGui::SameLine();
                        if(ImGui::Checkbox("Axes", &g_showAxes)) writeSave(options);
                        ImGui::SameLine();
                        if(ImGui::Checkbox("Limits", &g_showLimits)) writeSave(options);
                        ImGui::Separator();

                        if(ImGui::Checkbox("Connect Discontinuities",&interpolateDiscontinuities))
                        {
                            recalculateGraphs=true;
                        }
                        ImGui::SetItemTooltip(
                            "This calculator is stupid and doesn't actually know where exactly a discontinuity in a function like 1/x is.\nThus, it "
                            "tries to approximate it, but sometimes ends up creating visual artifacts in continuous functions.");
                        ImGui::SameLine();
                        if(ImGui::Checkbox("Mark Points",&g_markSpecialPoints)) writeSave(options);
                        ImGui::SetItemTooltip("Mark points where a function crosses 0, the point closest to the cursor, extremes.");

                        if(interpolateDiscontinuities) options.interpolateDiscontinuities=true;

                        // ImGui::Separator();
                        // ImGui::Text("X:");
                        // ImGui::SameLine();
                        // ImGui::Checkbox("Log##x",&logarithmicX);
                        // ImGui::Text("Y:");
                        // ImGui::SameLine();
                        // ImGui::Checkbox("Log##y",&logarithmicY);

                        ImGui::EndMenu();
                    }
                    static int decimalPlacesTextRnd{};

                    if(ImGui::BeginMenu("Misc."))
                    {

                        
                        
                        if(aroundTruthinessLeniencyNum.real()<0) 
                        {
                            aroundTruthinessLeniencyNum=0;
                            aroundTruthinessLeniency="0";
                        }
                        if(aroundTruthinessLeniencyNum.real()>1)
                        {
                            aroundTruthinessLeniencyNum=1;
                            aroundTruthinessLeniency="1";
                        }
                        options.aroundTruthinessLeniency=aroundTruthinessLeniencyNum;
                        if(ImGui::Checkbox("Prioritize Implicit Multiplication",&prioritizeImplicitMultiplication))
                        {
                            options.prioritizeImplicitMultiplication=prioritizeImplicitMultiplication;
                            writeSave(options);
                        }
                        ImGui::SetItemTooltip("Disambiguate something like 8÷2(2+2) as 8÷(2(2+2))=1 instead of (8÷2)(2+2)=16.");
                        ImGui::SameLine();
                        if(ImGui::Checkbox("Pretty Output",&prettyPrinting))
                        {
                            options.prettyPrinting=prettyPrinting;
                            writeSave(options);
                        }
                        ImGui::SetItemTooltip("Show some results as fractions or constants instead of decimal numbers and separate digits into groups of three.");
                        

                        ImGui::SetNextItemWidth(149.f);
                        // ImGui::SliderFloat("Max error for ≈",&aroundTruthinessLeniencyFloat,0,1);
                        
                        if(ImGui::InputText("≈ Tolerance", &aroundTruthinessLeniency,ImGuiInputTextFlags_CharsScientific))
                        {
                            if(g_formatInput) inputFormatter(aroundTruthinessLeniency,instances[currInst]);
                            lessetB::Options evalOptions{false,
                                                            0,
                                                            0,
                                                            0,
                                                            0,
                                                            false,
                                                            0,
                                                            "",
                                                            0,
                                                            ""};
                            std::string tmp=aroundTruthinessLeniency;
                            lessetB::evaluateEquation(evalOptions,true,false,tmp,nothing,aroundTruthinessLeniency,instances[currInst].customSymbols.m_variables, instances[currInst].customSymbols.m_functions,instances[currInst].customSymbols.m_operations);
                            if(lessetB::isRealNumber(aroundTruthinessLeniency) && aroundTruthinessLeniency!="" && !lessetB::globals::error)
                            {
                                aroundTruthinessLeniencyNum=static_cast<boost::multiprecision::cpp_complex<lessetB::maxPrecision>>(aroundTruthinessLeniency);
                            }
                        }
                        ImGui::SetItemTooltip("When 2 values are close to equal, what is the maximum difference for which ≈ returns true?");
                        ImGui::SameLine();
                        if(ImGui::Checkbox("Pretty Input",&g_formatInput)) writeSave(options);
                        ImGui::SetItemTooltip("Format your input (i.e. pi -> π)");

                        ImGui::SetNextItemWidth(315);
                        if(lessetB::globals::decimalPrecision!=lessetB::maxPrecision)
                            ImGui::DragInt("##", &lessetB::globals::decimalPrecision,0.2,0,lessetB::maxPrecision,"Decimal Places in Output: %d",ImGuiSliderFlags_AlwaysClamp);
                        else 
                        {
                            std::uniform_int_distribution<> intDist(0,1000);
                            if(decimalPlacesTextRnd==0) decimalPlacesTextRnd=intDist(lessetB::randomMt);
                            if(decimalPlacesTextRnd!=1000) ImGui::DragInt("##", &lessetB::globals::decimalPrecision,0.2,0,lessetB::maxPrecision,"Decimal Places in Output: Max",ImGuiSliderFlags_AlwaysClamp);
                            else 
                            {
                                decimalPlacesTextRnd=0;
                                ImGui::DragInt("##", &lessetB::globals::decimalPrecision,0.2,0,lessetB::maxPrecision,"Decimal Places in Output: All of 'em",ImGuiSliderFlags_AlwaysClamp);
                            }
                        }
                        ImGui::SetItemTooltip("Changes amount of decimal places shown in output.");
                        ImGui::SameLine();
                        if(lessetB::globals::separator==" ") lessetB::globals::separator="' '";
                        if(ImGui::Button(lessetB::globals::separator.c_str(),ImVec2(ROW_HEIGHT,ROW_HEIGHT)))
                        {
                            if(lessetB::globals::separator=="' '") lessetB::globals::separator=" ";
                            if(lessetB::globals::separator.at(0)=='\'') lessetB::globals::separator.at(0)=' ';
                            else if(lessetB::globals::separator.at(0)==' ') 
                            {
                                if(lessetB::globals::useDecimalComma) {lessetB::globals::separator.at(0)='\'';} 
                                else lessetB::globals::separator.at(0)=',';
                            }
                            else if(lessetB::globals::separator.at(0)=='.' || lessetB::globals::separator.at(0)==',') lessetB::globals::separator.at(0)='\'';
                            else lessetB::globals::separator='\'';
                            writeSave(options);
                        }
                        ImGui::SetItemTooltip("Separator character between digit groups.");
                        if(lessetB::globals::separator=="' '") lessetB::globals::separator=" ";
                        
                        

                        ImGui::EndMenu();
                    }
                    else decimalPlacesTextRnd=0;
                    
                    ImGui::Separator();

                    if(ImGui::BeginMenu("Style")) displayStyleMenu(style, options, gayMode, showGayMode, gayModeSpeed, hueModifier, saturationModifier);
                    
                    ImGui::EndMenu();
                }
                ImGui::SetItemTooltip("Tweak some settings.");

                if(ImGui::BeginMenu("History")) displayHistory(resultHistory, options);
                
                ImGui::SetItemTooltip("View past calculations.");

                if (ImGui::BeginMenu("Help"))
                {
                    if(ImGui::BeginMenu("Operators")) displayOperatorsHelpMenu(equation,instances[currInst].customSymbols.m_operations);

                    if(ImGui::BeginMenu("Functions"))
                    {
                        if(ImGui::BeginMenu("Common")) displayCommonHelpMenu(equation);
                        
                        if(ImGui::BeginMenu("Trig")) displayTrigHelpMenu(equation, tooltipRnd);
                        ImGui::SetItemTooltip("Squared variants (sin^2) exist too.");

                        if(ImGui::BeginMenu("Hyperbolic")) displayHyperbolicHelpMenu(equation);
                        ImGui::SetItemTooltip("Squared variants (sinh^2) exist too.");

                        if(ImGui::BeginMenu("Calculus")) displayCalculusHelpMenu(equation);

                        if(ImGui::BeginMenu("Num. Theory")) displayNumberTheoryHelpMenu(equation);

                        if(ImGui::BeginMenu("Statistics")) displayStatisticsHelpMenu(equation);

                        if(ImGui::BeginMenu("Complex")) displayComplexHelpMenu(equation);

                        if(ImGui::BeginMenu("Misc.")) displayMiscellaneousHelpMenu(equation);

                        if(ImGui::BeginMenu("Custom")) displayCustomFunctionsHelpMenu(equation, instances, currInst);

                        ImGui::EndMenu();
                    }

                    if(ImGui::BeginMenu("Logic")) displayLogicHelpMenu(equation);

                    ImGui::Separator();
                    displayTechincalHelpMenu();

                    if(ImGui::BeginMenu("Credits"))
                    {
                        ImGui::Text(
                            "Glued together by Dummigame.\nAlso responsible for the evaluator behind this n stuff.\nI am not that good at math btw. "
                            "If this thing tells you 2+2=5, call me.\nVery professional testing: Nyx.\n\nLibraries used in this project:\nImGui, "
                            "ImPlot, ImStyle, Boost lib, Native File Dialog Extended \n(MIT License, https://github.com/ocornut/imgui,\nMIT License, "
                            "https://github.com/epezent/implot,\nBSD 3-Clause License, https://github.com/csprite/ImStyle,\nBoost Software License, "
                            "https://www.boost.org/LICENSE_1_0.txt\nzlib License, https://github.com/btzy/nativefiledialog-extended)");
                        ImGui::EndMenu();
                    }
                    if(showTrivia)
                    {
                        if(ImGui::BeginMenu("Trivia"))
                        {
                            ImGui::Text("Technically, all predefined mutli argument functions are variadic, meaning you can supply an arbitrary "
                                        "number of arguments.\nWhether all of them are considered depends on the function.\n\nThis is my first "
                                        "decently large programming project, and, in fact, was never even meant to be that.\nThe original "
                                        "scope?\nCLI Tool with support for +-*/ and parentheses.");
                            ImGui::EndMenu();
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::SetItemTooltip("Learn about the calculator.");
                if(graphEquations.size()>=MANY_GRAPHS)
                {
                    if(ImGui::Checkbox("Draw many graphs",&drawMany_Graphs))
                    {
                        recalculateGraphs=true;
                    }
                }
                
                if(graphEquations.size() > previousGraphsEquationsSize)
                {
                    for(size_t i{}; i<graphEquations.size(); i++)
                    {
                        if(g_formatInput) inputFormatter(graphEquations[i], instances[currInst]);
                    }
                }
                previousGraphsEquationsSize=graphEquations.size();
                if(gayMode)
                {
                    for(size_t i{6}; i<ImGuiCol_COUNT; i++)
                    {
                        if(i==ImGuiCol_MenuBarBg) continue;
                        ImVec4 col{0,0,0,1};
                        ImGui::ColorConvertRGBtoHSV(style.Colors[i].x,style.Colors[i].y,style.Colors[i].z, col.x, col.y, col.z);
                        col.x=col.x+gayModeSpeed/1000;
                        if(col.x>1) col.x=0;
                        ImGui::ColorConvertHSVtoRGB( col.x, col.y, col.z,style.Colors[i].x,style.Colors[i].y,style.Colors[i].z);
                    }  
                }

                if(instances.size()>1)
                {
                    ImGui::Separator();
                    if(currInst!=0)
                    {
                        ImGui::Text("You are using instance %s.",instances[currInst].name.c_str());
                    }
                    else ImGui::Text("You are using the main instance.");
                }
                if(argc>1)
                {
                    debugMode=true;
                    static bool done{};
                    ImGui::Separator();
                    
                    #ifdef LESSET_INCLUDE_DEBUG
                    if(!done && !firstFrame)
                    {
                        lessetB::globals::debugCout=true;
                        std::string filePath(argv[1]);
                        runScript(graphEquations, options, instances, recalculateGraphs, currInst, filePath);
                        instances[currInst].hasRunScript=false;
                        std::cout<<instances[currInst].lastScriptOutput<<std::endl;
                        done=true;
                    }
                    #endif
                }
                #ifdef LESSET_INCLUDE_DEBUG
                if(lessetB::globals::debugCout) debugMode=true;
                if(debugMode)
                {
                    if(ImGui::BeginMenu("Debug"))
                    {
                        debugMenu(graphEquations, instances, currInst,graphsPoints);
                        maxIndividualGraphPoints=MAX_GRAPH_POINTS_BASE*g_maxIndividualGraphPointsMultiplier;
                    }
                }
                #endif
                ImGui::EndMenuBar();
            }

            
            updateSliders(timeStationary, instances, recalculateGraphs, currInst);
            

            style.FontScaleDpi = main_scale*2;
            ImGui::PushFont(io.FontDefault, 42.0f/2);      
            ImGui::SetNextItemWidth(io.DisplaySize.x/3);
            ImGui::InputText("##big guy",&equation); // The big one
            ImGui::PopFont();
            style.FontScaleDpi = main_scale;

            int unclosedParentheses{};
            if(g_formatInput) unclosedParentheses = inputFormatter(equation,instances[currInst]);
            else
            {
                std::string tmp=equation;
                unclosedParentheses=addClosingParentheses(tmp);
            }
            if(unclosedParentheses<0) ImGui::SetItemTooltip("Found extra closing parentheses.");
            
            if(equation!="") nonEmptyEquation=equation;
            std::string resultPlusEquals;
            if(equation!="") lastNonEmptyEquation=equation;
            else resultPlusEquals="";
            
            static boost::multiprecision::cpp_complex<lessetB::maxPrecision> resultNum{};
            if(result.find('\n')==std::string::npos) resultPlusEquals="  =  " + result;
            else resultPlusEquals="  =  ";
            static bool showStyleEditor{};
            #ifdef LESSET_INCLUDE_DEBUG
            if(equation=="fps")
            {
                resultPlusEquals = "  =  " + std::to_string(static_cast<int>(1.0/io.DeltaTime));
            }

            if(equation=="fpsavg")
            {
                resultPlusEquals = "  =  " + std::to_string(static_cast<int>(io.Framerate+.5));
            }
            #endif

            else if(equation=="how to exit vim" || equation=="how do i exit vim")
            {
                resultPlusEquals = "  =  :q";
            }

            else if(equation=="trivia")
            {
                showTrivia=true;
                resultPlusEquals = "  =  Read my ramblings! (help menu)";
            }

            else if(equation=="gayMode")
            {
                resultPlusEquals = "  =  check style options";
                showGayMode=true;
            }

            else if(equation.find("fishthumbs")!=std::string::npos)
            {
                resultPlusEquals = "  =  tacky could never";
                showGayMode=true;
            }

            else if(equation=="nine plus ten")
            {
                resultPlusEquals = "  =  twenty one";
            }
            #ifdef LESSET_INCLUDE_DEBUG
            else if(equation=="showStyleEditor")
            {
                showStyleEditor=true;
                resultPlusEquals = "  =  ask and you shall receive";
            }
            
            else if(equation=="enableDebug" || equation=="enαbleDebug")
            {
                if(equation=="enαbleDebug")
                    resultPlusEquals = "  =  Enαbled debug mode";
                else resultPlusEquals = "  =  Enabled debug mode";
                lessetB::globals::debugCout=true;
            }
            #endif

            bool textTooWide{};
            if(ImGui::CalcTextSize(resultPlusEquals.c_str()).x>io.DisplaySize.x/2)
            {
                textTooWide=true;
                ImGui::PushFont(io.FontDefault, 42.0f/(ImGui::CalcTextSize(resultPlusEquals.c_str()).x/io.DisplaySize.x*4));
            }
            
            ImGui::SameLine(io.DisplaySize.x/3+11);
            static bool errLabel{};
            if(ImGui::Button(resultPlusEquals.c_str(),ImVec2(0,48)))
            {
                resultWindow=true;
                result.clear();
                if(equation!="")
                {
                    errLabel=lessetB::evaluateEquation(options,false,false,nonEmptyEquation,resultHistory,result,instances[currInst].customSymbols.m_variables, instances[currInst].customSymbols.m_functions,instances[currInst].customSymbols.m_operations);
                    if(!containsX(equation, instances[currInst]))resultNum=lessetB::calculation<boost::multiprecision::cpp_complex<lessetB::maxPrecision>>(lessetB::getTokens(lessetB::cleanInput(lessetB::replaceUnicode(equation))),NAN);
                }
            } 
            

            if(result.find('\n')!=std::string::npos)
            {
                std::string label;
                if(errLabel) label="Errors";
                else label="Result";

                if(resultWindow)
                {
                    static bool hasShownHint{};
                    if(ImGui::Begin(label.c_str(),&resultWindow,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoDocking))
                    {
                        if(label=="Result")
                        {
                            if(showGraphingMenuHint) ImGui::MenuItem("For graphing, use the Graph menu!",NULL,false,false);
                            hasShownHint=true;
                        }
                        toCopyButtonEachLine(result);
                    }
                    else if(hasShownHint) showGraphingMenuHint=false;
                    ImGui::End();
                }
            }
            else if(floor(resultNum.real())==resultNum && !result.empty())
            {
                if(resultNum.real()<=LONG_LONG_MAX && resultNum.real()>=LONG_LONG_MIN)
                {
                    long long asInt = static_cast<size_t>(resultNum.real());
                    std::stringstream stream;
                    stream<<std::hex<<asInt;
                    std::string tooltip="Hex: 0x";
                    std::string hexRepresentation=stream.str();
                    for(size_t i{}; i<hexRepresentation.length(); i++)
                    {
                        if(hexRepresentation[i]>='a' && hexRepresentation[i]<='f') hexRepresentation[i]-='a'-'A';
                    }
                    tooltip+=hexRepresentation;
                    stream.str("");

                    stream<<std::oct<<asInt;
                    tooltip+="\nOct:  0";
                    tooltip+=stream.str();
                    stream.str("");
                    
                    stream<<std::bitset<sizeof(size_t)*8>(asInt);
                    tooltip+="\nBin:   0b";
                    std::string binaryRepresentation=stream.str();
                    while(binaryRepresentation.length() && binaryRepresentation[0]=='0') binaryRepresentation.erase(0,1);
                    tooltip+=binaryRepresentation;
                    if(binaryRepresentation.empty()) tooltip+="0";
                    ImGui::SetItemTooltip("%s", tooltip.c_str());
                }                
            }
            if(textTooWide) ImGui::PopFont();
            // if(graphsEquations.size()<MANY_GRAPHS && instances.size()==1) ImGui::Text("");




            const ImVec2 graphsPlotSize{io.DisplaySize.x+2,io.DisplaySize.y-85};
            if(!showStyleEditor)
            {
                ImPlotStyle* style = &ImPlot::GetStyle();
                style->MajorGridSize=ImVec2(0.01,0.01);
                // style->Colors[ImPlotCol_AxisGrid]=ImVec4(.2,.2,.2,1);
                style->PlotPadding=ImVec2(0,0);
            }

            if(showStyleEditor)
            {
                ImGui::Begin("Plot Style",&showStyleEditor, ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoDocking);
                ImPlot::ShowStyleEditor(&ImPlot::GetStyle());
                ImGui::End();
            }
            if(!showStyleEditor && equation=="showStyleEditor") equation.clear();


/*
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                                                Big plot area.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/
            ImGui::SetCursorPosX(-1);
            if(ImPlot::BeginPlot("Graphs",graphsPlotSize, ImPlotFlags_Equal|ImPlotFlags_NoTitle)) 
            {
                bool checkDiscontinuities{};
                setupPlot(defaultBoundsX, defaultBoundsY);
                static ImPlotRect limits{};
                ImPlotRect prevLimits {limits.X.Min, limits.X.Max, limits.Y.Min, limits.Y.Max};
                limits = ImPlot::GetPlotLimits();
                
                if(graphEquations.size()!=0)
                {
                    if(graphEquations.size()!=graphsPoints.first.size() || graphEquations.size()!=graphsPoints.second.size())
                    {
                        graphsPoints.first.resize(graphEquations.size());
                        graphsPoints.second.resize(graphEquations.size());
                    }


                    for(auto &eq : graphEquations)
                    {
                        if(eq.timePerPoint==std::chrono::nanoseconds()) eq.setTimePerPoint(instances[currInst]);
                    }

                    if(!(prevLimits.X.Min == limits.X.Min && prevLimits.X.Max == limits.X.Max))
                    {
                        timeStationary=0;
                    }
                    else timeStationary++;

                    const bool zoomedIn{abs(limits.X.Max-limits.X.Min)<abs(prevLimits.X.Max-prevLimits.X.Min)-0.000000001f};
                    size_t downsizeGraphIndex{static_cast<size_t>(-1)};
                    for(size_t j{}; j<graphsPoints.first.size(); j++)
                    {
                        auto &xCoordSet = graphsPoints.first[j];
                        // if(xCoordSet.size() > 2) std::cout<<abs(xCoordSet[1]-xCoordSet[0])/40<<'>'<<abs(limits.X.Max - limits.X.Min) / (maxIndividualGraphPoints*10) * (fmin(graphEquations[j].timePerPoint.count() / TARGET_PER_POINT_TIME + 0.5,MAX_POINT_REDUCTION))<<std::endl;
                        // std::cout<<std::endl<<"1:"<<12*abs(limits.X.Max-limits.X.Min)/(abs(limits.X.Max-limits.X.Min)/(maxIndividualGraphPoints / 10.) * (fmin(graphEquations.at(j).timePerPoint.count() / TARGET_PER_POINT_TIME + 0.5, MAX_POINT_REDUCTION)))
                        //          <<"\n2:"<< xCoordSet.size();
                        bool precisionTooHigh = xCoordSet.size() > 2 && (24*abs(limits.X.Max-limits.X.Min)/(abs(limits.X.Max-limits.X.Min)/(maxIndividualGraphPoints / 10.) * (fmin(graphEquations.at(j).timePerPoint.count() / TARGET_PER_POINT_TIME + 0.5, MAX_POINT_REDUCTION)))
                                 < xCoordSet.size() || abs(xCoordSet[1]-xCoordSet[0])/30 > abs(limits.X.Max - limits.X.Min) / (maxIndividualGraphPoints*10) * (fmin(graphEquations[j].timePerPoint.count() / TARGET_PER_POINT_TIME + 0.5,MAX_POINT_REDUCTION)));
                        if(precisionTooHigh)
                        {
                            downsizeGraphIndex=j;
                            break;
                        }
                    }

                    double minimumPrecision{(graphEquations.size()/2.f+1)};
                    // #warning The conditionals for triggering recalculation might be messed up
                    // Recalculate a graph when too many points have been saved for it... or recalculate the graph of recalculateGraphIndex. Only one graph per frame being recalculated is intentional.
                    if(
                        (
                            downsizeGraphIndex!=static_cast<size_t>(-1) && 
                            graphEquations.size()>downsizeGraphIndex && 
                            downsizeGraphIndex<graphsPoints.first.size() && 
                            graphsPoints.first.size()<MANY_GRAPHS && 
                            timeStationary<HIGH_PRECISION_DRAW_DELAY
                        ) || recalculateGraphIndex!=static_cast<size_t>(-1)
                    )
                    {
                        if(recalculateGraphIndex!=static_cast<size_t>(-1)) downsizeGraphIndex=recalculateGraphIndex;
                        {     
                            if(graphEquations.at(downsizeGraphIndex).fn.first.empty() && !recalculateGraphs && !graphEquations.at(downsizeGraphIndex).disabled && !graphEquations.at(downsizeGraphIndex).eq.empty())
                            {
                                #ifdef LESSET_INCLUDE_DEBUG
                                if(lessetB::globals::debugCout)
                                {
                                    std::cout<<"Recalculate single graph "<< graphEquations.at(downsizeGraphIndex).eq << '\n';
                                    lessetB::globals::debugCoutUsed=true;
                                }
                                #endif
                                nonEmptyGraphEquation=graphEquations.at(downsizeGraphIndex).eq;
                                lessetB::Options graphOptions{true,
                                                              limits.X.Min,
                                                              limits.X.Max,
                                                              abs(limits.X.Max - limits.X.Min) / (maxIndividualGraphPoints) *
                                                                    (fmin(graphEquations.at(downsizeGraphIndex).timePerPoint.count() / TARGET_PER_POINT_TIME + 0.5,MAX_POINT_REDUCTION)),
                                                              (aroundTruthinessLeniencyNum),
                                                              interpolateDiscontinuities,
                                                              prioritizeImplicitMultiplication,
                                                              graphEquations.at(downsizeGraphIndex).fn.first};
                                lessetB::evaluateEquation(graphOptions,true,false,nonEmptyGraphEquation,nothing,nothing,instances[currInst].customSymbols.m_variables, instances[currInst].customSymbols.m_functions,instances[currInst].customSymbols.m_operations);
                                checkDiscontinuities=true;
                                graphsPoints.first.at(downsizeGraphIndex)=lessetB::globals::points.first;
                                graphsPoints.second.at(downsizeGraphIndex)=lessetB::globals::points.second;
                            }       
                            else
                            {
                                graphsPoints.first.at(downsizeGraphIndex)=std::vector<double>();
                                graphsPoints.second.at(downsizeGraphIndex)=std::vector<double>();
                            }
                        }
                    }

                    // Quick, calculate the graph when the user re-enables it!
                    for(size_t i{}; i<graphEquations.size(); i++)
                    {
                        auto &currEq=graphEquations.at(i);
                        if(!currEq.disabled && graphsPoints.first.at(i).size()<=18 && !currEq.eq.empty())
                        {
                            {
                                #ifdef LESSET_INCLUDE_DEBUG
                                if(lessetB::globals::debugCout)
                                {
                                    std::cout<<"Reactivating graph "<< currEq.eq << '\n';
                                    lessetB::globals::debugCoutUsed=true;
                                }
                                #endif
                                nonEmptyGraphEquation=currEq.eq;
                                lessetB::Options graphOptions{true,
                                                              limits.X.Min,
                                                              limits.X.Max,
                                                              abs(limits.X.Max - limits.X.Min) / (maxIndividualGraphPoints / 10.) *
                                                                  (fmin(currEq.timePerPoint.count() / TARGET_PER_POINT_TIME + 0.5,MAX_POINT_REDUCTION)),
                                                              (aroundTruthinessLeniencyNum),
                                                              interpolateDiscontinuities,
                                                              prioritizeImplicitMultiplication,
                                                              currEq.fn.first};
                                lessetB::evaluateEquation(graphOptions,true,false,nonEmptyGraphEquation,nothing,nothing,instances[currInst].customSymbols.m_variables, instances[currInst].customSymbols.m_functions,instances[currInst].customSymbols.m_operations);
                                checkDiscontinuities=true;
                                graphsPoints.first.at(i)=lessetB::globals::points.first;
                                graphsPoints.second.at(i)=lessetB::globals::points.second;
                            }
                        }
                    }


                    // Prepend/Append to graph
                    if(timeStationary==0 && graphsPoints.first.size()<MANY_GRAPHS && !zoomedIn && !recalculateGraphs) 
                    {
                        const double dXMin{limits.X.Min-prevLimits.X.Min};
                        const double dXMinScreenProportion{abs(dXMin)/(limits.X.Max-limits.X.Min)};

                        const double dXMax{limits.X.Max-prevLimits.X.Max};
                        const double dXMaxScreenProportion{abs(dXMax)/(limits.X.Max-limits.X.Min)};
                        if(dXMin<0)
                            for(size_t j{}; j<graphEquations.size(); j++)
                            {
                                if(graphEquations.at(j).fn.first.empty() && !graphEquations.at(j).disabled && !graphEquations.at(j).eq.empty())
                                {
                                    if(graphsPoints.first.at(j).size()>0 && graphsPoints.first.at(j).at(0)>limits.X.Min)
                                    {
                                        #ifdef LESSET_INCLUDE_DEBUG
                                        // if(lessetB::globals::debugCout)
                                        // {
                                        //     std::cout<<"Prepending to graph "<< graphEquations.at(j).eq << '\n';
                                        //     lessetB::globals::debugCoutUsed=true;
                                        // }
                                        #endif
                                        nonEmptyGraphEquation=graphEquations.at(j).eq;
                                        
                                        lessetB::Options graphOptions{true,
                                                                    limits.X.Min,
                                                                    prevLimits.X.Min,
                                                                    abs(dXMin)/(maxIndividualGraphPoints*dXMinScreenProportion)*(fmin(graphEquations.at(j).timePerPoint.count()/TARGET_PER_POINT_TIME+0.5,MAX_POINT_REDUCTION)),
                                                                    (aroundTruthinessLeniencyNum),
                                                                    interpolateDiscontinuities,
                                                                    prioritizeImplicitMultiplication,
                                                                    graphEquations.at(j).fn.first};
                                        lessetB::evaluateEquation(graphOptions,true,false,nonEmptyGraphEquation,nothing,nothing,instances[currInst].customSymbols.m_variables, instances[currInst].customSymbols.m_functions,instances[currInst].customSymbols.m_operations);
                                        checkDiscontinuities=true;
                                        graphsPoints.first.at(j).insert_range(graphsPoints.first.at(j).begin(),lessetB::globals::points.first);
                                        graphsPoints.second.at(j).insert_range(graphsPoints.second.at(j).begin(),lessetB::globals::points.second);
                                    }
                                }
                                else
                                {
                                    graphsPoints.first.at(j)=std::vector<double>();
                                    graphsPoints.second.at(j)=std::vector<double>();
                                }
                            }
                        if(dXMax>0)
                            for(size_t j{}; j<graphEquations.size(); j++)
                            {
                                if(graphEquations.at(j).fn.first.empty() && !graphEquations.at(j).disabled && !graphEquations.at(j).eq.empty())
                                {
                                    if(graphsPoints.first.at(j).size()>0 && graphsPoints.first.at(j).at(graphsPoints.first.at(j).size()-1)<limits.X.Max)
                                    {
                                        #ifdef LESSET_INCLUDE_DEBUG
                                        // if(lessetB::globals::debugCout)
                                        // {
                                        //     std::cout<<"Appending to graph [ "<< graphEquations.at(j).eq << " ]\n";
                                        //     lessetB::globals::debugCoutUsed=true;
                                        // }
                                        #endif
                                        nonEmptyGraphEquation=graphEquations.at(j).eq;

                                        lessetB::Options graphOptions{true,
                                                                    prevLimits.X.Max,
                                                                    limits.X.Max,
                                                                    abs(dXMax)/(maxIndividualGraphPoints*dXMaxScreenProportion)*abs(fmin(graphEquations.at(j).timePerPoint.count()/TARGET_PER_POINT_TIME+0.5,MAX_POINT_REDUCTION)),
                                                                    (aroundTruthinessLeniencyNum),
                                                                    interpolateDiscontinuities,
                                                                    prioritizeImplicitMultiplication,
                                                                    graphEquations.at(j).fn.first};
                                        lessetB::evaluateEquation(graphOptions,true,false,nonEmptyGraphEquation,nothing,nothing,instances[currInst].customSymbols.m_variables, instances[currInst].customSymbols.m_functions,instances[currInst].customSymbols.m_operations);
                                        checkDiscontinuities=true;
                                        graphsPoints.first.at(j).append_range(lessetB::globals::points.first);
                                        graphsPoints.second.at(j).append_range(lessetB::globals::points.second);
                                    }
                                }
                                else
                                {
                                    graphsPoints.first.at(j)=std::vector<double>();
                                    graphsPoints.second.at(j)=std::vector<double>();
                                }
                            }

                    }
                    // Recalculate when zooming in
                    else if(timeStationary==0 && graphsPoints.first.size()<MANY_GRAPHS && zoomedIn && !recalculateGraphs)
                    {
                        #ifdef LESSET_INCLUDE_DEBUG
                        if(lessetB::globals::debugCout)
                        {
                            std::cout<<"Recalculating from zooming in\n";
                            lessetB::globals::debugCoutUsed=true;
                        }
                        #endif
                        std::vector<char> wouldIncreasePrecision(graphsPoints.first.size());
                        size_t i{};
                        for(auto &xCoordSet : graphsPoints.first)
                        {
                            wouldIncreasePrecision[i]=(xCoordSet.size() < 2 ||
                                (abs(limits.X.Max - limits.X.Min) / (maxIndividualGraphPoints / 10.) * (fmin(graphEquations.at(i).timePerPoint.count() / TARGET_PER_POINT_TIME + 0.5, MAX_POINT_REDUCTION))
                                 < abs(xCoordSet[xCoordSet.size()/2+1] - xCoordSet[xCoordSet.size()/2])));
                            i++;
                        }
                        for(size_t j{}; 
                            j<graphEquations.size() && 
                            !(prevLimits.X.Min == limits.X.Min && prevLimits.X.Max == limits.X.Max); 
                            j++)
                        {

                            if(graphEquations.at(j).fn.first.empty() && !graphEquations.at(j).disabled && !graphEquations.at(j).eq.empty())
                            {
                                if(j>=wouldIncreasePrecision.size() || wouldIncreasePrecision.at(j)==true)
                                {
                                    #ifdef LESSET_INCLUDE_DEBUG
                                    if(lessetB::globals::debugCout)
                                    {
                                        std::cout<<"Recalculating graph "<<graphEquations.at(j).eq<<'\n';
                                        lessetB::globals::debugCoutUsed=true;
                                    }
                                    #endif
                                    nonEmptyGraphEquation=graphEquations.at(j).eq;

                                    lessetB::Options graphOptions{true,
                                                                limits.X.Min,
                                                                limits.X.Max,
                                                                abs(limits.X.Max - limits.X.Min) / (maxIndividualGraphPoints / 10.) *
                                                                    (fmin(graphEquations.at(j).timePerPoint.count() / TARGET_PER_POINT_TIME + 0.5,MAX_POINT_REDUCTION)),
                                                                (aroundTruthinessLeniencyNum),
                                                                interpolateDiscontinuities,
                                                                prioritizeImplicitMultiplication,
                                                                graphEquations.at(j).fn.first};
                                    lessetB::evaluateEquation(graphOptions,true,false,nonEmptyGraphEquation,nothing,nothing,instances[currInst].customSymbols.m_variables, instances[currInst].customSymbols.m_functions,instances[currInst].customSymbols.m_operations);
                                    checkDiscontinuities=true;
                                    graphsPoints.first[j]=lessetB::globals::points.first;
                                    graphsPoints.second[j]=lessetB::globals::points.second;
                                }
                            }
                            else
                            {
                                graphsPoints.first[j]=std::vector<double>();
                                graphsPoints.second[j]=std::vector<double>();
                            }
                        }
                    }
                    // Recalculate at a high precision
                    if(timeStationary==HIGH_PRECISION_DRAW_DELAY || recalculateGraphs)
                    {
                        timeStationary=HIGH_PRECISION_DRAW_DELAY;
                        #ifdef LESSET_INCLUDE_DEBUG
                        if(lessetB::globals::debugCout)
                        {
                            std::cout<<"Recalculating at high precision\n";
                            lessetB::globals::debugCoutUsed=true;
                        }
                        #endif

                        std::vector<char> wouldIncreasePrecision(graphsPoints.first.size());
                        if(!recalculateGraphs)
                        {
                            size_t i{};
                            for(auto &xCoordSet : graphsPoints.first)
                            {
                                wouldIncreasePrecision[i]=(xCoordSet.size() < 2 ||
                                    (abs(limits.X.Max - limits.X.Min) / maxIndividualGraphPoints * minimumPrecision *
                                    (fmin(graphEquations.at(i).timePerPoint.count() / TARGET_PER_POINT_TIME + 0.5,MAX_POINT_REDUCTION))
                                    < abs(xCoordSet[xCoordSet.size()/2+1] - xCoordSet[xCoordSet.size()/2])));
                                i++;
                            }
                        }
                        else wouldIncreasePrecision.clear();
                        if((graphEquations.size()>=MANY_GRAPHS && drawMany_Graphs) || graphEquations.size()<MANY_GRAPHS)
                        {
                            for(size_t j{}; j<graphEquations.size(); j++)
                            {

                                if(graphEquations.at(j).fn.first.empty() && !graphEquations.at(j).disabled && !graphEquations.at(j).eq.empty())
                                {
                                    if(j>=wouldIncreasePrecision.size() || wouldIncreasePrecision.at(j)==true || recalculateGraphs)
                                    {
                                        #ifdef LESSET_INCLUDE_DEBUG
                                        if(lessetB::globals::debugCout)
                                        {
                                            std::cout<<"Recalculating graph "<<graphEquations.at(j).eq<<'\n';
                                            lessetB::globals::debugCoutUsed=true;
                                        }
                                        #endif
                                        nonEmptyGraphEquation=graphEquations.at(j).eq;
                                        lessetB::Options graphOptions{true,
                                                                    limits.X.Min,
                                                                    limits.X.Max,
                                                                    abs(limits.X.Max - limits.X.Min) / maxIndividualGraphPoints * minimumPrecision *
                                                                        (fmin(graphEquations.at(j).timePerPoint.count() / TARGET_PER_POINT_TIME + 0.5,MAX_POINT_REDUCTION)),
                                                                    (aroundTruthinessLeniencyNum),
                                                                    interpolateDiscontinuities,
                                                                    prioritizeImplicitMultiplication,
                                                                    graphEquations.at(j).fn.first};
                                        lessetB::evaluateEquation(graphOptions,true,false,nonEmptyGraphEquation,nothing,nothing,instances[currInst].customSymbols.m_variables, instances[currInst].customSymbols.m_functions,instances[currInst].customSymbols.m_operations);
                                        checkDiscontinuities=true;
                                        graphsPoints.first[j]=lessetB::globals::points.first;
                                        graphsPoints.second[j]=lessetB::globals::points.second;
                                    }
                                }
                                else
                                {
                                    graphsPoints.first[j]=std::vector<double>();
                                    graphsPoints.second[j]=std::vector<double>();
                                }
                            }
                        }
                    } 

                    ImPlotSpec spec{};
                    spec.Flags=ImPlotItemFlags_NoFit;
                    spec.LineWeight=2.f;
                    ImPlotPoint mousePointTextLocation;
                    SpecialPoint closestSpecialPointToCursor;
                    bool hasShownMousePointText{};
                    for(size_t j{}; j<graphsPoints.first.size(); j++) // Disconnect discontinuities and hand points to ImPlot
                    {
                       
                        if(!isCurrentPlotHidden()) graphEquations.at(j).disabled=false;
                        if((graphEquations.size()>=MANY_GRAPHS && timeStationary<HIGH_PRECISION_DRAW_DELAY) || (graphEquations.size()>=MANY_GRAPHS && !drawMany_Graphs)) break;
                        if(lessetB::isEmptyEquation(graphEquations.at(j).eq) || !graphEquations.at(j).fn.first.empty()) 
                        {
                            continue;
                        }
                        if(graphEquations.size()<MANY_GRAPHS && !interpolateDiscontinuities)
                        {
                            for(size_t i{}; i<graphsPoints.first.at(j).size() && (checkDiscontinuities || recalculateGraphs || recalculateGraphIndex==j || timeStationary==0 || timeStationary==HIGH_PRECISION_DRAW_DELAY); i++)
                            {
                                if(i+1<graphsPoints.first.at(j).size())
                                {
                                    if(graphsPoints.first.at(j).at(i)>graphsPoints.first.at(j).at(i+1))
                                    {
                                        graphsPoints.second.at(j).erase(graphsPoints.second.at(j).begin()+i);
                                        graphsPoints.first.at(j).erase(graphsPoints.first.at(j).begin()+i);
                                    }
                                }
                                if(i>0 && i<graphsPoints.first.at(j).size()-2)
                                {
                                    const float previousDifference = (graphsPoints.second.at(j).at(i)-graphsPoints.second.at(j).at(i-1)) / (graphsPoints.first.at(j).at(i)-graphsPoints.first.at(j).at(i-1));
                                    const float difference = (graphsPoints.second.at(j).at(i+1)-graphsPoints.second.at(j).at(i)) / (graphsPoints.first.at(j).at(i+1)-graphsPoints.first.at(j).at(i));
                                    const float nextDifference = (graphsPoints.second.at(j).at(i+2)-graphsPoints.second.at(j).at(i+1)) / (graphsPoints.first.at(j).at(i+2)-graphsPoints.first.at(j).at(i+1));

                                    // Since this is merely an approximation of the derivative, it'll go crazy at a discontinuity, which is why the following conditions work.
                                    if(!interpolateDiscontinuities &&
                                       ((difference != 0 && previousDifference == 0) ||
                                        // get rid of the next line if able
                                        (abs(difference) > abs(previousDifference) * MAX_CHANGE_FACTOR_SECOND && abs(difference) > abs(nextDifference) * MAX_CHANGE_FACTOR_SECOND) ||
                                        (
                                            (difference > previousDifference * MAX_CHANGE_FACTOR_FIRST && difference > 0 && previousDifference > 0) ||
                                            (difference < previousDifference * MAX_CHANGE_FACTOR_FIRST && difference < 0 && previousDifference < 0) ||
                                            (difference > -previousDifference * MAX_CHANGE_FACTOR_FIRST && difference < 0 && previousDifference > 0) ||
                                            (-difference > previousDifference * MAX_CHANGE_FACTOR_FIRST && difference > 0 && previousDifference < 0)
                                        )) &&
                                       abs(difference) / abs(limits.Y.Max - limits.Y.Min) > 0.03 // Don't see differences near 0 as discontinuities
                                    )
                                    {
                                        if(!isNoisy(graphsPoints.first.at(j),graphsPoints.second.at(j),i,g_maxIndividualGraphPointsMultiplier))
                                        {
                                            // Basically prevent ImPlot from interpolating between points
                                            graphsPoints.second.at(j).at(i)=NAN;
                                            // if(graphEquations.size()>20)
                                            // {
                                            //     graphsPoints.second.at(j).at(i)=NAN;
                                            // }
                                            // else
                                            // {
                                                
                                            //     graphsPoints.second.at(j).insert(graphsPoints.second.at(j).cbegin()+i,NAN);
                                            //     graphsPoints.first.at(j).insert(graphsPoints.first.at(j).cbegin()+i,NAN);
                                            //     i++;
                                            //     graphsPoints.second.at(j).insert(graphsPoints.second.at(j).cbegin()+i+1,NAN);
                                            //     graphsPoints.first.at(j).insert(graphsPoints.first.at(j).cbegin()+i+1,NAN);
                                            //     i++;
                                            // }
                                        }
                                    }
                                }                    
                            }
                        }

                        spec.LineColor=graphEquations.at(j).col;
                        if(timeStationary<HIGH_PRECISION_DRAW_DELAY) ImPlot::PlotLine(std::string(graphEquations.at(j).eq+"##"+std::to_string(j)).c_str(), &(*graphsPoints.first.at(j).cbegin()), &(*graphsPoints.second.at(j).cbegin()), graphsPoints.second.at(j).size(),spec);
                        else if((drawMany_Graphs && graphEquations.size()>=MANY_GRAPHS) || graphEquations.size()<MANY_GRAPHS) ImPlot::PlotLine(std::string(graphEquations.at(j).eq+"##"+std::to_string(j)).c_str(), &(*graphsPoints.first.at(j).cbegin()), &(*graphsPoints.second.at(j).cbegin()), graphsPoints.second.at(j).size(),spec);


                        ImPlotSpec pointsSpec{};
                        pointsSpec.Flags=ImPlotItemFlags_NoFit;
                        pointsSpec.MarkerLineColor=ImVec4{0,0,0,.7};
                        pointsSpec.MarkerFillColor=ImVec4(1,1,1,1);
                        if(!isPreviousPlotHidden())
                        {
                            graphEquations.at(j).disabled=false;
                            if(g_markSpecialPoints && graphEquations.size() < 25 &&  ImPlot::IsPlotHovered())
                            {
                                auto temp=markPoints(graphEquations, timeStationary, graphsPoints, options, limits, j, pointsSpec, hasShownMousePointText,closestSpecialPointToCursor);
                                if(temp.x==temp.x)
                                {
                                    mousePointTextLocation=temp;
                                }
                            }
                        }
                        else graphEquations.at(j).disabled=true;
                    }
                    if(!closestSpecialPointToCursor.eq.empty())
                    {
                        
                        bool textBelow = mousePointTextLocation.y+limits.Y.Max>closestSpecialPointToCursor.y;
                        ImPlot::PushStyleColor(ImPlotCol_InlayText,closestSpecialPointToCursor.col);
                        std::string coordsFormatted = formatCoords(GraphEquation(closestSpecialPointToCursor.eq), closestSpecialPointToCursor.x, closestSpecialPointToCursor.y, options);
                        ImPlot::PlotText(coordsFormatted.c_str(), closestSpecialPointToCursor.x, closestSpecialPointToCursor.y, ImVec2(0, 30*(2*textBelow-1)));
                        ImPlot::PushStyleColor(ImPlotCol_InlayText,ImVec4(1,1,1,1));
                    }
                }
                
                if(g_showAxes) 
                {
                    displayAxisUI();
                    displayGridUI(limits);
                }
                if(g_showLimits) displayLimitsUI(limits,options);
                ImPlot::EndPlot();
            }

            if(defaultFirstGraphColor && graphEquations.size()>0) // Adjust first graph color with color changes if left default
            { 
                float alpha = graphEquations.at(0).col.w;
                graphEquations.at(0).col=style.Colors[ImGuiCol_Button];
                graphEquations.at(0).col.w=alpha;
            }
            recalculateGraphs=false;
            ImGui::End();
        }
        #ifdef LESSET_INCLUDE_DEBUG
        if(lessetB::globals::debugCoutUsed) std::cout<<'\n';
        #endif

        // Rendering
        ImGui::Render();        ImDrawData* main_draw_data = ImGui::GetDrawData();        const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);        wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;        wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;        wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;        wd->ClearValue.color.float32[3] = clear_color.w;        if (!main_is_minimized)FrameRender(wd, main_draw_data);
        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable && false)        {            ImGui::UpdatePlatformWindows();            ImGui::RenderPlatformWindowsDefault();        }
        // Present Main Platform Window
        if (!main_is_minimized) FramePresent(wd);
        if(!main_is_minimized && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Q)) break;

        firstFrame=false;
    }
    // Cleanup
    std::cout<<"\nBye Bye!\n";
    err = vkDeviceWaitIdle(g_Device);    check_vk_result(err);    ImGui_ImplVulkan_Shutdown();    ImGui_ImplGlfw_Shutdown();    ImPlot::DestroyContext();    ImGui::DestroyContext();CleanupVulkanWindow(&g_MainWindowData);CleanupVulkan();glfwDestroyWindow(window);glfwTerminate();
    writeSave(options);
    return 0;
}

/*
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                                          Welcome to Function Definition Hell.
                                                    Enjoy your stay.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
*/


bool isNoisy(const std::vector<double> &pointsX, const std::vector<double> &pointsY, size_t i, int maxIndividualGraphPointsMultiplier)
{
    int switches{};
    bool rising{};
    bool prevRising{};
    size_t j=i-10;
    if(i<10) return false;
    {
        for(; j<i+10 && j+1<pointsX.size(); j++)
        {
            if(pointsX.at(j)==0) return true;
            // if(j>0 && pointsX.at(j)-pointsX.at(j+1) != pointsX.at(j-1)-pointsX.at(j)) return true;
            if(pointsY.at(j)<pointsY.at(j+1))
            {
                rising=true;
                if(prevRising!=rising) switches++;
                if(j==i-10) switches--;
            }
            else if(pointsY.at(j)>pointsY.at(j+1))
            {
                rising=false;
                if(prevRising!=rising) switches++;
            }
            prevRising=rising;
        } 
    }

    if(switches<6) return false;

    return true; 
}

char convertToHexDigit(char8_t byte)
{
    if(byte>15)
    {
        return '\0';
    }
    else
    {
        if(byte<=9)
        {
            return byte+'0';
        }
        else return byte-10+'A';
    }
}

std::string colVecToHex(ImVec4 col)
{
    // This is ass.
    std::string hexStr;
    if(col.x>1) col.x=1.f;
    if(col.y>1) col.y=1.f;
    if(col.z>1) col.z=1.f;
    if(col.w>1) col.w=1.f;
    char8_t byte = col.x*255;
    hexStr.push_back(convertToHexDigit(byte/16)); // Greater Digit
    hexStr.push_back(convertToHexDigit(byte%16)); // Lesser Digit
    byte = col.y*255;
    hexStr.push_back(convertToHexDigit(byte/16));
    hexStr.push_back(convertToHexDigit(byte%16));
    byte = col.z*255;
    hexStr.push_back(convertToHexDigit(byte/16));
    hexStr.push_back(convertToHexDigit(byte%16));
    byte = col.w*255;
    hexStr.push_back(convertToHexDigit(byte/16));
    hexStr.push_back(convertToHexDigit(byte%16));
    return hexStr;
}

void writeGraphsScript(const std::string &path,const std::vector<GraphEquation> &graphEquations)
{
    std::ofstream script(path);
    bool empty{true};

    size_t i{};
    for(GraphEquation item : graphEquations)
    {
        if(item.fn.second!=lessetB::Function())
        {
            empty=false;
            script<<"fn "<< item.fn.first << "(";
            for(size_t i{}; i<item.fn.second.argNames.size(); i++)
            {
                script<<item.fn.second.argNames.at(i);
                if(i+1<item.fn.second.argNames.size()) script<<',';
            }
            script<<") = "<<item.fn.second.definition;
            script<<'\n';
        }
        else if(!item.eq.empty())
        {
            empty=false;
            script<<"graph "<<item.eq;
            script<<'\n';
            script<<"color "<<i<<"=#"+colVecToHex(item.col)<<'\n';
        }
        i++;


    }
    script.flush();
}


void writeGraphsScript(const std::string &path,const std::vector<GraphEquation> &graphEquations, const std::string &filename)
{
    std::ofstream script(path+filename);
    bool empty{true};

    size_t i{};
    for(GraphEquation item : graphEquations)
    {
        if(item.fn.second!=lessetB::Function())
        {
            empty=false;
            script<<"fn "<< item.fn.first << "(";
            for(size_t i{}; i<item.fn.second.argNames.size(); i++)
            {
                script<<item.fn.second.argNames.at(i);
                if(i+1<item.fn.second.argNames.size()) script<<',';
            }
            script<<") = "<<item.fn.second.definition;
            script<<'\n';
        }
        else if(!item.eq.empty())
        {
            empty=false;
            script<<"graph "<<item.eq;
            script<<'\n';
            script<<"color "<<i<<"=#"+colVecToHex(item.col)<<'\n';
        }
        i++;


    }

    std::cout<<std::filesystem::relative(path)<<std::endl;

    if(std::filesystem::exists(std::filesystem::relative(path)))
    {
        std::filesystem::remove(std::filesystem::relative(path));
    }

    if(std::filesystem::exists(filename) && empty)
    {
        std::filesystem::remove(filename);
    }
    script.flush();
}

void writeSave(lessetB::Options &options)
{
    if(std::filesystem::exists("LessetSave.txt"))
    {
        std::filesystem::remove("LessetSave.txt");
    }
    std::ofstream settings("LessetSave.txt");
    settings<<
            lessetB::globals::useDecimalComma<<" <= Use decimal comma\n"<<
            options.prettyPrinting<<" <= Pretty printing\n"<<
            options.prioritizeImplicitMultiplication<<" <= Prioritize implicit multiplication\n"<<
            lessetB::globals::separator<<" <= Separator character for digit groups (this may be a space!)\n"<<
            lessetB::globals::decimalPrecision<<" <= Precision\n"<<
            g_maxIndividualGraphPointsMultiplier<<" <= Graphing resolution (1-5)\n"<<
            g_showAxes<<" <= Show Axes in Plot\n"<<
            g_showLimits<<" <= Show limits of plot view\n"<<
            g_markSpecialPoints<< " <= Mark special points\n"<<
            g_formatInput<< " <= Format Input\n";
    settings.close();
}

void readSave(lessetB::Options &options)
{
    if(!std::filesystem::exists("LessetSave.txt")) return;
    if(!std::filesystem::is_regular_file("LessetSave.txt")) return;

    std::ifstream settings("LessetSave.txt");
    char useDecimalComma = settings.get();
    lessetB::globals::useDecimalComma = !!(useDecimalComma-'0');
    for(size_t i{}; i<20000 && settings.get()!='\n'; i++) if(i==20000-1) return;

    char prettyPrinting = settings.get();
    if(settings.fail()) return;
    options.prettyPrinting=!!(prettyPrinting-'0');
    for(size_t i{}; i<20000 && settings.get()!='\n'; i++) if(i==20000-1) return;
    
    char prioritizeImplicitMultiplication = settings.get();
    if(settings.fail()) return;
    options.prioritizeImplicitMultiplication=!!(prioritizeImplicitMultiplication-'0');
    for(size_t i{}; i<20000 && settings.get()!='\n'; i++) if(i==20000-1) return;
    
    char separator = settings.get();
    if(!(separator==',' || separator=='\'' || separator==' ') || (separator==',' && lessetB::globals::useDecimalComma)) separator='\'';
    if(settings.fail()) return;
    lessetB::globals::separator=separator;
    for(size_t i{}; i<20000 && settings.get()!='\n'; i++) if(i==20000-1) return;

    std::string precision;
    for(size_t i{}; i<5; i++)
    {
        char c = settings.get();
        if(settings.fail()) return;
        if(!std::isdigit(c)) break;
        else precision.push_back(c);
    }

    int precisionNum = std::stoi(precision);
    if(precisionNum>lessetB::maxPrecision) precisionNum=lessetB::maxPrecision;
    lessetB::globals::decimalPrecision=precisionNum;

    for(size_t i{}; i<20000 && settings.get()!='\n'; i++) if(i==20000-1) return;

    char resolutionChar = settings.get();

    int resolution = (resolutionChar-'0')%6;
    if(resolution<=0) resolution=2;
    g_maxIndividualGraphPointsMultiplier=resolution;

    for(size_t i{}; i<20000 && settings.get()!='\n'; i++) if(i==20000-1) return;

    char showAxesChar = settings.get();
    if(settings.fail()) return;
    g_showAxes=!!(showAxesChar-'0');

    for(size_t i{}; i<20000 && settings.get()!='\n'; i++) if(i==20000-1) return;

    char showLimitsChar = settings.get();
    if(settings.fail()) return;
    g_showLimits=!!(showLimitsChar-'0');

    for(size_t i{}; i<20000 && settings.get()!='\n'; i++) if(i==20000-1) return;

    char markPoints = settings.get();
    if(settings.fail()) return;
    g_markSpecialPoints=!!(markPoints-'0');


    for(size_t i{}; i<20000 && settings.get()!='\n'; i++) if(i==20000-1) return;

    char format = settings.get();
    if(settings.fail()) return;
    g_formatInput=!!(format-'0');

}

int addClosingParentheses(std::string &equation)
{                
    int unclosedParentheses{};
    for(size_t i{}; i<equation.length(); i++)
    {
        if(equation.at(i)=='(') unclosedParentheses++;
        else if(equation.at(i)==')') unclosedParentheses--;
    }
    if(unclosedParentheses>0)
    {
        while(unclosedParentheses>0)
        {
            equation.push_back(')');
            unclosedParentheses--;
        }
    }
    return unclosedParentheses;
}
void cleanWhitespace(std::string &input)
{

    // This code belongs in the gallery of gross incompetence
    
    for(size_t i{}; i<input.length(); i++) if(input.at(i)=='\t') input.erase(i--,1);

    if(input.length()>sizeof(U' '))
    {
        for(size_t i{}; i<input.length(); i++) 
        {
            if(input.at(i)==' ')
            {
                input.replace(i,1," ");
                i+=sizeof(U' ');
            }
        }
    }

    std::u32string uStr(input.begin(),input.end());
    while(uStr.length()>0 && std::isspace(uStr.at(0))) uStr.erase(0,1);
    for(size_t i{}; i<uStr.length(); i++)
    {
        if(std::isspace(uStr.at(i))) uStr.at(i)=U' ';
    }
    for(size_t i{1}; i<uStr.length(); i++)
    {
        if((uStr.at(i-1)==U' ' || std::isspace(uStr.at(i-1))) && (uStr.at(i)==U' ' || std::isspace(uStr.at(i)))) uStr.erase(--i,1); 
    }
    input.assign(uStr.begin(),uStr.end());
    if(input.length()>sizeof(U' '))
    {
        for(size_t i{}; i<input.length()-sizeof(U' '); i++)
        {
            if(input.find(" ",i)==i && input.find(" ",i+sizeof(U' ')-1)==i+sizeof(U' ')-1) 
            {
                input.erase(i,sizeof(U' ')-1);
                i-=sizeof(U' ')-1;
            }
        }
    }

    while(input.length()>2 && input.find(" ",0)==0) input.erase(0,sizeof(U' ')-1);
    while(input.length()>2 && input.find(" ",input.size()-sizeof(U' ')+1)==input.size()-sizeof(U' ')+1) input.erase(input.size()-sizeof(U' ')+1,sizeof(U' ')-1);
    for(int i{}; i<input.length(); i++)
    {
        if(std::isspace(input.at(i))) input.erase(i--,1);
    }
    
}
void addSeparatorsAfterFunctions(std::string &input, const Instance &instance, std::string separator)
{
    std::u32string uStr(input.begin(),input.end());
    for(size_t i{}; i<uStr.length(); i++)
    {
        std::string candidate;
        size_t j{MAX_KEYWORD_LENGTH};
        if(j>input.length()-i) j=input.length()-i;

        for(; j>0; j--)
        {
            candidate = input.substr(i,j);
            // std::cout<<candidate<<'\n';
            if((lessetB::globals::multiArgFunctions.find(candidate)!=lessetB::globals::multiArgFunctions.end() && input.length()>i+candidate.length() && input[i+candidate.length()]=='(') || 
               instance.customSymbols.m_functions.find(candidate)!=instance.customSymbols.m_functions.end() || 
               instance.customSymbols.m_variables.find(candidate)!=instance.customSymbols.m_variables.end())
            {
                i+=candidate.length();
                break;
            }
            if(lessetB::globals::symbols.find(candidate)!=lessetB::globals::symbols.end() || instance.customSymbols.m_operations.find(candidate)!=instance.customSymbols.m_operations.end())
            {
                using enum lessetB::operatorAttributes;
                if((instance.customSymbols.m_operations.find(candidate)!=instance.customSymbols.m_operations.end() && instance.customSymbols.m_operations.at(candidate).m_info==(U_PREFIX|R_ASSOCIATIVE) && instance.customSymbols.m_operations.at(candidate).m_precedence==lessetB::pass::FUNCTIONS) || 
                   (lessetB::globals::symbols.find(candidate)!=lessetB::globals::symbols.end() && lessetB::globals::symbols.at(candidate)==lessetB::tokenType::FUNCTION))
                {
                    i+=candidate.length();
                    if(i>=input.length()) break;
                    else if(uStr.find(std::u32string(separator.begin(),separator.end()),i)!=i && uStr.at(i)!=U'(')
                    {
                        input.insert(i, separator);
                        uStr.insert(i,std::u32string(separator.begin(),separator.end()));
                        i+=separator.length()-1;
                        break;
                    }
                }
            }
        }
    }
    input.assign(uStr.begin(),uStr.end());
}
bool isCurrentPlotHidden()
{
    ImPlotContext &imPlotContext = *GImPlot;
    if(imPlotContext.CurrentItem)
    {
        return !imPlotContext.CurrentItem->Show;
    }
    else return false;
}

bool isPreviousPlotHidden()
{
    ImPlotContext &imPlotContext = *GImPlot;
    if(imPlotContext.PreviousItem)
    {
        return !imPlotContext.PreviousItem->Show;
    }
    else return false;
}

void displayAxisUI()
{
    const float axisVals[]{0};
    ImPlotSpec axisSpec{};
    axisSpec.LineColor=ImVec4(1,1,1,.4);
    axisSpec.Flags|=ImPlotItemFlags_NoLegend|ImPlotItemFlags_NoFit;

    ImPlot::PlotInfLines("##1",axisVals,1,axisSpec);

    axisSpec.Flags|=ImPlotInfLinesFlags_Horizontal;
    ImPlot::PlotInfLines("##2",axisVals,1,axisSpec);
}

void displayGridUI(const ImPlotRect &limits)
{
    return;
    // Wip
    assert(false && "Don't call me!");
    constexpr size_t targetLines=24;
    double rangeX = (limits.X.Max-limits.X.Min);
    double segmentWidthX=lessetB::round(rangeX/targetLines,static_cast<size_t>(rangeX/10));
    double gridVals[targetLines]{};

    double location{limits.X.Min};


    for(size_t i{0};i<targetLines; i++)
    {
        gridVals[i]=location;
        location+=segmentWidthX;
    }
    ImPlotSpec axisSpec{};
    axisSpec.LineColor=ImVec4(1,1,1,.2);
    axisSpec.LineWeight=.3;
    axisSpec.Flags|=ImPlotItemFlags_NoLegend|ImPlotItemFlags_NoFit;

    ImPlot::PlotInfLines("##1",gridVals,targetLines,axisSpec);

    axisSpec.Flags|=ImPlotInfLinesFlags_Horizontal;

    double rangeY = (limits.Y.Max-limits.Y.Min);
    double segmentWidthY=lessetB::round(rangeY/targetLines/((limits.Y.Max-limits.Y.Min)/(limits.X.Max-limits.X.Min)),static_cast<size_t>(rangeY));

    location=limits.Y.Min;


    for(size_t i{0};i<targetLines; i++)
    {
        gridVals[i]=location;
        location+=segmentWidthY;
    }


    ImPlot::PlotInfLines("##2",gridVals,targetLines,axisSpec);
}

void displayLimitsUI(const ImPlotRect& limits, lessetB::Options &options)
{
    ImPlotSpec textSpec{};
    textSpec.Flags|=ImPlotItemFlags_NoFit;
    std::string asStr;
    // This is ass.
    
    if(abs(limits.X.Max)<1e5) 
        asStr=lessetB::formatNumber(std::format("{: .2f}",limits.X.Max), options,true,true);
    else 
        asStr=lessetB::formatNumber(std::format("{: .5}",limits.X.Max), options,true,true);

    ImPlot::PlotText(asStr.c_str(),limits.X.Max,limits.Y.Min+abs(limits.Y.Max-limits.Y.Min)/2,ImVec2((-ImGui::CalcTextSize(asStr.c_str()).x/2-5)*(-1+!(ImPlot::GetCurrentPlot()->Axes[0].Flags&ImPlotAxisFlags_Invert)*2),0),textSpec);
    
    if(abs(limits.X.Min)<1e5) 
        asStr=lessetB::formatNumber(std::format("{: .2f}",limits.X.Min), options,true,true);
    else 
        asStr=lessetB::formatNumber(std::format("{: .5}",limits.X.Min), options,true,true);
    ImPlot::PlotText(asStr.c_str(),limits.X.Min,limits.Y.Min+abs(limits.Y.Max-limits.Y.Min)/2,ImVec2((ImGui::CalcTextSize(asStr.c_str()).x/2+5)*(-1+!(ImPlot::GetCurrentPlot()->Axes[0].Flags&ImPlotAxisFlags_Invert)*2),0),textSpec);
    
    if(abs(limits.Y.Min)<1e5) 
        asStr=lessetB::formatNumber(std::format("{: .2f}",limits.Y.Min), options,true,true);
    else 
        asStr=lessetB::formatNumber(std::format("{: .5}",limits.Y.Min), options,true,true);
    ImPlot::PlotText(asStr.c_str(),limits.X.Min+abs(limits.X.Max-limits.X.Min)/2,limits.Y.Min,ImVec2(0,-15*(-1+!(ImPlot::GetCurrentPlot()->Axes[3].Flags&ImPlotAxisFlags_Invert)*2)),textSpec);
    
    if(abs(limits.Y.Max)<1e5) 
        asStr=lessetB::formatNumber(std::format("{: .2f}",limits.Y.Max), options,true,true);
    else 
        asStr=lessetB::formatNumber(std::format("{: .5}",limits.Y.Max), options,true,true);
    ImPlot::PlotText(asStr.c_str(),limits.X.Min+abs(limits.X.Max-limits.X.Min)/2,limits.Y.Max,ImVec2(0,15*(-1+!(ImPlot::GetCurrentPlot()->Axes[3].Flags&ImPlotAxisFlags_Invert)*2)),textSpec);
}

bool ImVec4Equal(const ImVec4 &a, const ImVec4 &b, bool cmpAlpha)
{
    return a.x==b.x && a.y==b.y && a.z==b.z && (cmpAlpha ? a.w==b.w : true);
} 

std::string removeSpaces(std::string s)
{
    for(size_t i{}; i<s.length(); i++)
    {
        if(std::isspace(s[i])) s.erase(i--,1);
    }
    return s;
}

void replaceWithUnicode(std::string &input, 
                        const std::unordered_map<std::string, lessetB::tokenType> &symbols, 
                        const std::unordered_map<std::string, lessetB::tokenType> &multiArgFunctions,
                        const lessetB::CustomSymbols &customSymbols) 
{
    std::string candidate;
    for (size_t i{}; i < input.length(); i++)
    {
        {

            candidate.clear();
            for(size_t j{MAX_KEYWORD_LENGTH}; j>0; j--)
            {
                if(j>input.length()-1) j=input.length()-i;
                candidate=input.substr(i,j);
                if(symbols.find(candidate)!=symbols.end() || 
                   customSymbols.m_variables.find(candidate)!=customSymbols.m_variables.end() ||
                   customSymbols.m_operations.find(candidate)!=customSymbols.m_operations.end() ||
                   multiArgFunctions.find(candidate)!=multiArgFunctions.end())
                {
                    if(multiArgFunctions.find(candidate)!=multiArgFunctions.end())
                    {
                        if((i+candidate.length()+1<input.length() && input.at(i+candidate.length()+1)=='(')) 
                        {
                            i+=candidate.length();
                            break;
                        }
                        else if((i+candidate.length()+1>=input.length())) break;
                    }
                    if(lessetB::globals::asciiToUnicode.find(candidate) == lessetB::globals::asciiToUnicode.end())
                        i+=candidate.length()-1;
                    break;
                }
            }
        }
        if(lessetB::globals::asciiToUnicode.find(candidate) != lessetB::globals::asciiToUnicode.end()) 
            input.replace(i,candidate.length(),lessetB::globals::asciiToUnicode.at(candidate));
    }
}

bool isPartOfHexValue(const std::string &str)
{
    for(char c : str)
    {
        if(c>='0' && c<='F' && !(c>'9' && c<'A')) continue;
        return false;
    }
    return true;
}

bool convertHexToByte(const std::string &str, char8_t &byte)
{
    if(str.length()<2) return true;
    char c=str[0];
    if(c>='0' && c<='9') byte=(c-'0')*16;
    else if(c>='A' && c<='F') byte=(c-'A'+10)*16;
    c=str[1];
    if(c>='0' && c<='9') byte+=(c-'0');
    else if(c>='A' && c<='F') byte+=(c-'A'+10);
    return false;
}

template<typename T>
requires std::is_arithmetic_v<T>
ImVec4 ImVec4ScalarDiv(ImVec4 vec, T num)
{
    return ImVec4(vec.x/num,vec.y/num,vec.z/num,vec.w/num);
}


template<typename T>
requires std::is_arithmetic_v<T>
ImVec4 ImVec4ScalarMul(ImVec4 vec, T num)
{
    return ImVec4(vec.x*num,vec.y*num,vec.z*num,vec.w*num);
}

template <typename T>
requires std::is_arithmetic_v<T>
[[nodiscard]] ImVec4 ImVec4ScalarAdd(ImVec4 vec, T num)
{
    return ImVec4(vec.x+num,vec.y+num,vec.z+num,vec.w+num);
};


void runScript(std::vector<GraphEquation> &graphEquations, lessetB::Options &options, std::vector<Instance> &instances, bool &recalculateGraphs, 
                  size_t &currInst, std::string &filePath, bool restricted, bool writeOutput)
{
    size_t colorsOffset=graphEquations.size();
    std::string nothing;
    size_t iterations{};
    std::ifstream calculationsFile;
    calculationsFile.open(filePath);
    if(!calculationsFile.fail())
    {
        if(!instances[currInst].lastScriptOutput.empty())
        {
            instances[currInst].lastScriptOutput.clear();
        }
        std::string currLine;
        bool skip{};
        bool conditionTrue{};
        instances[currInst].hasRunScript = true;
        while(std::getline(calculationsFile, currLine) && iterations <= MAX_COMMANDS)
        {
            nothing.clear();
            std::string conditionValue;
            std::string jumpValue;
            iterations++;
            for(size_t i{}; i < currLine.length() && currLine.at(i) != '#'; i++)
            {
                if(std::isspace(currLine.at(i))) currLine.erase(i--, 1);
            }

            if(currLine.find("let") == 0 && currLine.find('=') != std::string::npos && !restricted && (!skip || conditionTrue == true))
            {
                std::string name = currLine.substr(3, currLine.find('=') - 3);
                if(name.length() > MAX_KEYWORD_LENGTH)
                {
                    if(writeOutput)
                        instances[currInst].lastScriptOutput += "Names cannot be longer than 33 characters.";
                    continue;
                }
                std::string eq = currLine.substr(currLine.find('=') + 1);
                if(instances[currInst].customSymbols.m_operations.find(name)!=instances[currInst].customSymbols.m_operations.end())
                {
                    instances[currInst].lastScriptOutput += "Failed assignment " + currLine + ", variables and operators may not share names.\n";
                }
                else if(addVariable(std::pair<std::string, std::string>(name, eq), instances[currInst]))
                {
                    if(writeOutput)
                        instances[currInst].lastScriptOutput += "Failed assignment " + currLine + '\n';
                }
                else if(writeOutput)
                    instances[currInst].lastScriptOutput +=
                        "Assigned " + name + " value " + instances[currInst].customSymbols.m_variables.at(name) + '\n';
                continue;
            }
            if(currLine.find("endIF") == 0  && !restricted)
            {
                skip = false;
                if(writeOutput)
                    instances[currInst].lastScriptOutput += "endIF\n";
                continue;
            }

            if(currLine.find("graph") == 0 && (!skip || conditionTrue == true))
            {
                for(auto var : instances[currInst].customSymbols.m_variables)
                {
                    for(size_t i{}; i < currLine.length(); i++)
                    {
                        if(currLine.find("numof", i) == i)
                        {
                            if(currLine.find(var.first) <= i + 6)
                            {
                                currLine.replace(i, 5 + var.first.length(), var.second);
                                if(colorsOffset>0) colorsOffset--;
                            }
                        }
                    }
                }

                if(0 == graphEquations.size())
                {
                    if(writeOutput)
                        instances[currInst].lastScriptOutput += "Graphing " + currLine.substr(5) + '\n';
                    graphEquations.push_back({currLine.substr(5), std::pair<std::string, lessetB::Function>()});
                    recalculateGraphs = true;
                }
                else
                    for(size_t i{}; i < graphEquations.size(); i++)
                    {
                        if(graphEquations.at(i).eq == currLine.substr(5)) break;
                        else if(i >= graphEquations.size() - 1)
                        {
                            if(writeOutput)
                                instances[currInst].lastScriptOutput += "Graphing " + currLine.substr(5) + '\n';
                            graphEquations.push_back({currLine.substr(5), std::pair<std::string, lessetB::Function>()});
                            recalculateGraphs = true;
                        }
                    }
                continue;
            }

            if(currLine.find("color") == 0 && (!skip || conditionTrue == true))
            {
                if(currLine.find("=")!=std::string::npos && currLine.find("#")!=std::string::npos && currLine.find("#")>currLine.find("="))
                {
                    std::string indexStr = currLine.substr(5,currLine.find("=")-5);
                    try
                    {
                        int index = std::stoi(indexStr)+colorsOffset;
                        if(index<graphEquations.size())
                        {
                            ImVec4 newCol{};
                            std::string hexCodeStr=currLine.substr(currLine.find("#")+1);
                            for(size_t i{}; i<=6; i+=2)
                            {
                                std::string val;
                                val=hexCodeStr.substr(i,2);
                                if(isPartOfHexValue(val) && val.length()==2)
                                {
                                    char8_t byte{};
                                    if(!convertHexToByte(val, byte))
                                    {
                                        if(writeOutput) instances[currInst].lastScriptOutput += val;
                                        float asFloat = byte;
                                        switch(i)
                                        {
                                            case 0: newCol.x=asFloat; break;
                                            case 2: newCol.y=asFloat; break;
                                            case 4: newCol.z=asFloat; break;
                                            case 6: newCol.w=asFloat; break;
                                        }

                                    }
                                }
                            

                            }
                            graphEquations.at(index).col=ImVec4ScalarDiv(newCol,255.f);
                            if(writeOutput) instances[currInst].lastScriptOutput +="\n";
                            
                        }

                    }
                    catch(...)
                    {
                        if(writeOutput) instances[currInst].lastScriptOutput += "Something went wrong\n";
                    }
                }
                continue;
            }
            if(currLine.find("op") == 0 && (!skip || conditionTrue == true))
            {
                std::string opSig = currLine.substr(2, currLine.find('=') - 2);
                bool hasLparen{};
                bool hasRparen{};
                bool hasLbracket{};
                bool hasRbracket{};
                bool failed{};
                size_t commaCount{};
                for(char c : opSig)
                {
                    if(opSig.find('=') != std::string::npos)
                    {
                        if(writeOutput) instances[currInst].lastScriptOutput += "Operator definition failed, bad signature.\n";
                        failed = true;
                    }
                    if(c == '(')
                    {
                        if(hasLparen || hasRparen || hasLbracket || hasRbracket)
                        {
                            if(writeOutput) instances[currInst].lastScriptOutput += "Operator definition failed, bad signature.\n";
                            failed = true;
                        }
                        hasLparen = true;
                    }
                    if(c == ',')
                    {
                        if(!hasLparen || commaCount>1)
                        {
                            if(writeOutput) instances[currInst].lastScriptOutput += "Operator definition failed, bad signature.\n";
                            failed = true;
                        }
                        commaCount++;
                    }
                    if(c == ')')
                    {
                        if(hasRparen || !hasLparen || hasLbracket || hasRbracket)
                        {
                            if(writeOutput) instances[currInst].lastScriptOutput += "Operator definition failed, bad signature.\n";
                            failed = true;
                        }
                        hasRparen = true;
                    }
                    if(c == '[')
                    {
                        if(hasLbracket || !hasLparen || !hasRparen || hasRbracket)
                        {
                            if(writeOutput) instances[currInst].lastScriptOutput += "Operator definition failed, bad signature.\n";
                            failed = true;
                        }
                        hasLbracket = true;
                    }
                    if(c == ']')
                    {
                        if(hasRbracket || !hasLbracket || !hasLparen || !hasRparen)
                        {
                            if(writeOutput) instances[currInst].lastScriptOutput += "Operator definition failed, bad signature.\n";
                            failed = true;
                        }
                        hasRbracket = true;
                    }
                }
                if(!(hasLbracket && hasRbracket && hasLparen && hasRparen))
                {
                    if(writeOutput) instances[currInst].lastScriptOutput += "Operator definition failed, bad signature.\n";
                    continue;
                }
                std::string opName = opSig.substr(0, opSig.find('('));
                if(opName.length() > MAX_KEYWORD_LENGTH)
                {
                    if(writeOutput) instances[currInst].lastScriptOutput += "Operator definition failed, names cannot be longer than 33 characters.\n";
                    continue;
                }
                
                for(size_t i{}; i < opName.length(); i++)
                {
                    if(!lessetB::isValidName(opName) || i >= MAX_KEYWORD_LENGTH)
                    {
                        failed = true;
                        if(writeOutput) instances[currInst].lastScriptOutput += "Operator definition failed, name contained invalid characters\n";
                        break;
                    }
                }
                std::string argNames = opSig.substr(opSig.find('('));
                std::string attributesStr = currLine.substr(currLine.find('['),currLine.find(']')-currLine.find('['));
                lessetB::operatorAttributes attributes{};
                size_t precedence{};
                if(attributesStr.find(',')==std::string::npos || attributesStr.length()<4)
                {
                    if(writeOutput) instances[currInst].lastScriptOutput += "Operator definition failed, bad signature.\n";
                    continue;
                }
                {
                    if((attributesStr[1]-'0')%2==1)
                    {
                        attributes=attributes|lessetB::operatorAttributes::R_ASSOCIATIVE;
                    }
                    if(attributesStr[1]-'0'>1)
                    {
                        if(argNames.find(',')==std::string::npos) attributes=attributes|lessetB::operatorAttributes::U_POSTFIX;
                        
                    }
                    else if(argNames.find(',')==std::string::npos) attributes=attributes|lessetB::operatorAttributes::U_PREFIX;
                    std::string precedenceStr=attributesStr.substr(attributesStr.find(',')+1);

                    std::from_chars(precedenceStr.data(), precedenceStr.data() + precedenceStr.length(), precedence);
                    if(precedence+3>lessetB::pass::CS18)
                    {
                        if(writeOutput) instances[currInst].lastScriptOutput += "Operator definition failed, bad precedence (0-34).\n";
                        continue;
                    }
                    precedence+=3;
                }
                std::string opDef = currLine.substr(currLine.find('=')+1); 
                auto err = getOperationFromArgs(opSig, opDef, static_cast<lessetB::pass>(precedence), attributes, instances[currInst]);
                if(std::to_underlying(err) && writeOutput)
                {
                    using enum symbolCreationError;
                    switch(err)
                    {
                        case BAD_NAME: instances[currInst].lastScriptOutput += "Bad operator name.\n"; break;
                        case BAD_PARAMS: instances[currInst].lastScriptOutput += "Bad parameter name(s).\n"; break;
                        case BAD_DEFINITION: instances[currInst].lastScriptOutput += "Bad definition.\n"; break;
                        case DOUBLED_CUSTOM_NAME: instances[currInst].lastScriptOutput += "Cannot reuse variable name.\n"; break;
                        case BAD_SIGNATURE: instances[currInst].lastScriptOutput += "Bad signature.\n"; break;
                        default: instances[currInst].lastScriptOutput += "what\n"; break;
                    }
                }
                else 
                {
                    instances[currInst].lastScriptOutput += "Defined operator  \"" + opName + "\" = " + opDef+"\n";
                }
                continue;

            }
            if(currLine.find("fn") == 0 && (!skip || conditionTrue == true))
            {
                std::string fnSig = currLine.substr(2, currLine.find('=') - 2);
                bool hasLparen{};
                bool hasRparen{};
                bool failed{};
                for(char c : fnSig)
                {
                    if(fnSig.find('=') != std::string::npos)
                    {
                        if(writeOutput) instances[currInst].lastScriptOutput += "Function definition failed, bad signature.\n";
                        failed = true;
                    }
                    if(c == '(')
                    {
                        if(hasLparen)
                        {
                            if(writeOutput) instances[currInst].lastScriptOutput += "Function definition failed, bad signature.\n";
                            failed = true;
                        }
                        hasLparen = true;
                    }
                    if(c == ')')
                    {
                        if(hasRparen)
                        {
                            if(writeOutput) instances[currInst].lastScriptOutput += "Function definition failed, bad signature.\n";
                            failed = true;
                        }
                        hasRparen = true;
                    }
                }
                std::string fnName = fnSig.substr(0, fnSig.find('('));
                if(fnName.length() > MAX_KEYWORD_LENGTH)
                {
                    if(writeOutput) instances[currInst].lastScriptOutput += "Function definition failed, names cannot be longer than 33 characters.\n";
                    continue;
                }

                for(size_t i{}; i < fnName.length(); i++)
                {
                    if(!lessetB::isValidName(fnName) || i >= MAX_KEYWORD_LENGTH)
                    {
                        failed = true;
                        if(writeOutput) instances[currInst].lastScriptOutput += "Function definition failed, name contained invalid characters\n";
                        break;
                    }
                }

                std::string definition = currLine.substr(currLine.find('=') + 1);
                if(definition == "")
                {
                    if(writeOutput) instances[currInst].lastScriptOutput += "Function definition failed, empty definition\n";
                    failed = true;
                }
                lessetB::Function fn = getFnFromSignature(currLine.substr(2),instances[currInst]);
                if(fn.argNames.size() == 0)
                {
                    if(writeOutput) instances[currInst].lastScriptOutput += "Function definition failed\n";
                    failed = true;
                }
                if(failed) continue;

                if(lessetB::globals::multiArgFunctions.find(fnName) == lessetB::globals::multiArgFunctions.end() &&
                   (lessetB::globals::symbols.find(fnName) == lessetB::globals::symbols.end() ||
                    lessetB::globals::symbols.find(fnName)->second != lessetB::tokenType::FUNCTION))
                {
                    if(instances[currInst].customSymbols.m_functions.find(fnName) != instances[currInst].customSymbols.m_functions.end())
                    {
                        if(instances[currInst].customSymbols.m_functions.find(fnName)->second.definition != definition)
                            instances[currInst].customSymbols.m_functions.at(fnName) = fn;
                    }
                    else instances[currInst].customSymbols.m_functions.emplace(fnName, fn);
                    instances[currInst].protectedFunctionDefinitions.emplace(fnName, 'c');
                    if(writeOutput) instances[currInst].lastScriptOutput += "Defined function " + fnSig + " = " + definition + '\n';
                    continue;
                }
                else
                {
                    if(writeOutput) instances[currInst].lastScriptOutput += "Function definition failed, cannot shadow predefined functions\n";
                    continue;
                }
            }

            if(currLine.find("IF") == 0  && !restricted)
            {
                std::string subEquation = currLine.substr(2);
                if(subEquation != "")
                {
                    for(size_t i{}; i < subEquation.length(); i++)
                    {
                        if(std::isspace(subEquation.at(i)))
                        {
                            subEquation.erase(i--, 1);
                        }
                    }
                    lessetB::Options ifOptions = options;
                    ifOptions.prettyPrinting = false;
                    evaluateEquation(ifOptions, true, false, subEquation, nothing, conditionValue, instances[currInst].customSymbols.m_variables,
                                     instances[currInst].customSymbols.m_functions, instances[currInst].customSymbols.m_operations);

                    if(lessetB::isRealNumber(conditionValue) &&
                       static_cast<boost::multiprecision::cpp_complex<lessetB::maxPrecision>>(conditionValue).real() >= 1)
                    {
                        conditionTrue = true;
                    }
                    if(calculationsFile.peek() == '\n')
                        for(; calculationsFile.peek() == '\n'; calculationsFile.seekg(static_cast<size_t>(calculationsFile.tellg()) + 1));
                    skip = true;
                    if(conditionTrue && writeOutput) instances[currInst].lastScriptOutput += "beginIF\n";
                    continue;
                }
            }
            if(currLine.find("endIF") == 0 && conditionTrue && !restricted)
            {
                conditionTrue = false;
            }

            if(currLine.find("jump") == 0 && (!skip || conditionTrue == true) && !restricted)
            {
                std::string jumpEquation = currLine.substr(4);
                if(jumpEquation != "")
                {
                    long long jumpDestination{};
                    for(size_t i{}; i < jumpEquation.length(); i++)
                    {
                        if(std::isspace(jumpEquation.at(i)))
                        {
                            jumpEquation.erase(i--, 1);
                        }
                    }

                    lessetB::Options jumpOptions = options;
                    jumpOptions.prettyPrinting = false;
                    evaluateEquation(jumpOptions, true, false, jumpEquation, nothing, jumpValue, instances[currInst].customSymbols.m_variables,
                                     instances[currInst].customSymbols.m_functions, instances[currInst].customSymbols.m_operations); // Lines with # are comments

                    std::from_chars(jumpValue.data(), jumpValue.data() + jumpValue.length(), jumpDestination);

                    calculationsFile.seekg(std::ios::beg);
                    if(jumpDestination < 1) jumpDestination = 1;
                    for(long long i{}; i < jumpDestination - 1; i++)
                    {
                        calculationsFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    }

                    if(calculationsFile.peek() == '\n')
                        for(; calculationsFile.peek() == '\n'; calculationsFile.seekg(static_cast<size_t>(calculationsFile.tellg()) + 1));
                    conditionTrue = false;
                    skip = false;
                    if(writeOutput)
                        instances[currInst].lastScriptOutput += "Jumped to line " + std::to_string(jumpDestination) + '\n';
                    continue;
                }
            }

            if(!skip || conditionTrue == true && !restricted && writeOutput) // Calculating kinda useless if the output is discarded anyway
            {
                if(writeOutput)
                    evaluateEquation(options, true, true, currLine, nothing, instances[currInst].lastScriptOutput,
                                     instances[currInst].customSymbols.m_variables, instances[currInst].customSymbols.m_functions,instances[currInst].customSymbols.m_operations); // Lines with # are comments
            }
            if(calculationsFile.peek() == '\n')
                while(calculationsFile.peek() == '\n') calculationsFile.seekg(static_cast<size_t>(calculationsFile.tellg()) + 1);
        }
        calculationsFile.close();
    }
    else std::cout<<"Couldn't open "<<filePath<<std::endl;
}


void displayHistory(std::string &resultHistory, lessetB::Options &options)
{

    size_t width = calcMaxLineButtonWidth(resultHistory);

    if(resultHistory != "")
    {
        if(ImGui::Button("Clear", ImVec2(width, ROW_HEIGHT)))
        {
            resultHistory.clear();
            lessetB::globals::tokenMemory.clear();
            lessetB::globals::ans.clear();
            options.ans = "nan";
        }
    }
    else
    {
        ImGui::MenuItem("You have no history.", NULL, false, false);
    }

    toCopyButtonEachLine(resultHistory, width);

    ImGui::EndMenu();
}

void displayOperatorsHelpMenu(std::string &equation, std::unordered_map<std::string, lessetB::Operation>& operationContainer)
{
    if(ImGui::MenuItem("+")) equation.append("+");
    ImGui::SetItemTooltip("Adds left and right or does nothing (unary plus). Is a tooltip really needed for this..?");

    if(ImGui::MenuItem("-")) equation.append("-");
    ImGui::SetItemTooltip("Subtracts left and right or negates right.");

    if(ImGui::MenuItem("×")) equation.append("×");
    ImGui::SetItemTooltip("Multiplication. (*)");

    if(ImGui::MenuItem("/")) equation.append("/");
    ImGui::SetItemTooltip("Division. Please, not by 0. (÷)");

    if(ImGui::MenuItem("//")) equation.append("//");
    ImGui::SetItemTooltip("Floor division. Only works on real numbers.");

    if(ImGui::MenuItem("^")) equation.append("^");
    ImGui::SetItemTooltip("Exponentiation. (**)");

    if(ImGui::MenuItem("!")) equation.append("!");
    ImGui::SetItemTooltip("Factorial.");

    if(ImGui::MenuItem("!!")) equation.append("!!");
    ImGui::SetItemTooltip("Double-Factorial.");

    if(ImGui::MenuItem("mod")) equation.append("mod");
    ImGui::SetItemTooltip("Modulus, division with remainder. (%%)\nFloors the result of the division. fmod truncates it, rmod rounds it.");

    if(ImGui::MenuItem("nPk")) equation.append("nPk");
    ImGui::SetItemTooltip("Permutations.");

    if(ImGui::MenuItem("nCk")) equation.append("nCk");
    ImGui::SetItemTooltip("Binomial coefficient.");

    if(ImGui::MenuItem("|expr|")) equation.append("||");
    ImGui::SetItemTooltip("Absolute value.");

    if(ImGui::MenuItem("(expr)")) equation.append("()");
    ImGui::SetItemTooltip("Parentheses.");

    if(lessetB::globals::userOperations.size()>0) ImGui::Separator();
    for(auto op :operationContainer)
    {
        if(ImGui::MenuItem(op.first.c_str())) equation.append(op.second.m_def);
        std::string argNames;
        for(auto &argName : op.second.m_argNames)
        {
            if(!argName.empty())
            {
                argNames+=argName;
                argNames+=", ";
            }
        }

        std::string tooltip = std::format("A {}-associative {}{} operator with precedence {} of {}returns {}",
                                            op.second.m_info & lessetB::R_ASSOCIATIVE ? "right" : "left",
                                            op.second.m_info & lessetB::BINARY ? "" : (op.second.m_info & lessetB::U_PREFIX ? "prefix " : "postfix "),
                                            op.second.m_info & lessetB::BINARY ? "binary" : "unary",
                                            getPriorityName(op.second.m_precedence).c_str(),
                                            argNames,
                                            op.second.m_def
                                            );
        ImGui::SetItemTooltip("%s",tooltip.c_str());
        // ImGui::SetItemTooltip("%s, Evaluated on pass %s, takes %d argument%s, is %s-associative%s", 
        //                       op.second.m_def.c_str(),
        //                       getPriorityName(op.second.m_precedence).c_str(), 
        //                       (op.second.m_info & lessetB::BINARY) + 1,
        //                       op.second.m_info & lessetB::BINARY ? "s" : "",
        //                       op.second.m_info & lessetB::R_ASSOCIATIVE ? "right" : "left",
        //                       op.second.m_info & lessetB::BINARY ? "" : (op.second.m_info & lessetB::U_PREFIX ? "Prefix" : "Postfix"));
    }

    ImGui::EndMenu();
}


void displayCustomFunctionsHelpMenu(std::string &equation, std::vector<Instance> &instances, size_t &currInst)
{
    {
        if(instances[currInst].customSymbols.m_functions.size() == 0) ImGui::Text("You can make custom functions in the Graph menu.");
        size_t i{};
        for(auto fn : instances[currInst].customSymbols.m_functions)
        {
            std::string argNames;
            for(size_t j{}; j < instances[currInst].customSymbols.m_functions.at(fn.first).argNames.size(); j++)
            {
                std::string argName = instances[currInst].customSymbols.m_functions.at(fn.first).argNames.at(j);
                argNames += argName;
                if(j < instances[currInst].customSymbols.m_functions.at(fn.first).argNames.size() - 1) argNames += ",";
            }
            if(ImGui::MenuItem(std::string(fn.first + "(" + argNames + ")##" + std::to_string(i)).c_str()))
            {
                equation.append(fn.second.definition);
            }
            ImGui::SetItemTooltip("%s", fn.second.definition.c_str());
            i++;
        }
        ImGui::EndMenu();
    }
}

#include "help_menus.hpp"

void toCopyButtonEachLine(std::string &str)
{
    std::string line;
    // ImGui::Text("%s",resultHistory.c_str());

    for(size_t i{}; i < str.length();)
    {
        line = str.substr(i, str.find('\n', i) - i) + "##" + std::to_string(i);
        if(ImGui::Button(line.c_str()))
        {
            ImGui::SetClipboardText(str.substr(i, str.find('\n', i) - i).c_str());
        }
        ImGui::SetItemTooltip("Click to copy.");
        i += line.length() - 1 - std::to_string(i).length();
    }
}

void toCopyButtonEachLine(std::string &resultHistory, size_t width)
{
    std::string line;
    for(size_t i{1}; i < resultHistory.length(); i += line.length() + 1)
    {
        line = resultHistory.substr(i, resultHistory.find('\n', i) - i);
        if(ImGui::Button(line.c_str(), ImVec2(width, ROW_HEIGHT)))
        {
            ImGui::SetClipboardText(line.c_str());
        }
        ImGui::SetItemTooltip("Click to copy.");
    }
}

[[nodiscard]] size_t calcMaxLineButtonWidth(std::string &str)
{
    std::string line;

    size_t width{};
    for(size_t i{1}; i < str.length(); i += line.length() + 1)
    {
        line=str.substr(i, str.find('\n', i) - i);
        width = fmax(width, ImGui::CalcTextSize(line.c_str()).x + 20);
    }
    return width;
}

std::string getPriorityName(const lessetB::pass priority)
{
    std::string name;
    using namespace lessetB;
    switch(priority)
    {
        case CS1: name="Custom 1"; break;
        case CS2: name="Custom 2"; break;
        case CS3: name="Custom 3"; break;
        case CS4: name="Custom 4"; break;
        case CS5: name="Custom 5"; break;
        case CS6: name="Custom 6"; break;
        case CS7: name="Custom 7"; break;
        case CS8: name="Custom 8"; break;
        case CS9: name="Custom 9"; break;
        case CS10: name="Custom 10"; break;
        case CS11: name="Custom 11"; break;
        case CS12: name="Custom 12"; break;
        case CS13: name="Custom 13"; break;
        case CS14: name="Custom 14"; break;
        case CS15: name="Custom 15"; break;
        case CS16: name="Custom 16"; break;
        case CS17: name="Custom 17"; break;
        case CS18: name="Custom 18"; break;
        case INVALID: name="Invalid"; break;
        case INITIAL: name="Initial"; break;
        case SUBEXPRESSIONS: name="Subexpressions"; break;
        case FUNCTIONS: name="Functions"; break;
        case UNARY_OPS: name="Factorials"; break;
        case EXPONENTIATION: name="Exponentiation"; break;
        case UNARY_MINUS: name="Unary Minus"; break;
        case NOT: name="Not"; break;
        case MULTIPLICATION_IMPLICIT: name="Implicit Multiplication"; break;
        case MULTIPLICATION: name="Multiplication";  break;
        case ADDITION: name="Addition"; break;
        case BITWISE0: name="Bitwise Shift"; break;
        case SPACESHIP: name="Spaceship"; break;
        case BITWISE1: name="Bitwise AND"; break;
        case BITWISE2: name="Bitwise XOR"; break;
        case BITWISE3: name="Bitwise OR"; break;
        case COMPARISONS1: name="Comparisons"; break;
        case COMPARISONS2: name="Equality"; break;
        case LOGIC1: name="Logical AND"; break;
        case LOGIC2: name="Logical XOR"; break;
        case LOGIC3: name="Logical OR"; break;
    };
    return name;

}

void resetGraphingData(std::vector<GraphEquation> &graphEquations, std::pair<std::vector<std::vector<double>>, std::vector<std::vector<double>>> &graphsPoints)
{
    graphEquations.clear();
    graphsPoints.first.clear();
    graphsPoints.second.clear();

    graphEquations.shrink_to_fit();
    graphsPoints.first.shrink_to_fit();
    graphsPoints.second.shrink_to_fit();

    lessetB::globals::points.first.clear();
    lessetB::globals::points.second.clear();
    lessetB::globals::tokenMemory.clear();
}

[[nodiscard]] std::string formatCoords(const GraphEquation &graphEquation, double x, double y, const lessetB::Options &options, bool withEquation)
{
    if(abs(x)<1e5 && abs(y)<1e5)
        return (withEquation ? graphEquation.eq : "")+"\n(" +   lessetB::formatNumber(std::format("{:< 10.2f}",x),options,true,true)+ "; " + lessetB::formatNumber(std::format("{:> 10.2f}",y),options,true,true) + ")";
    else return (withEquation ? graphEquation.eq : "")+"\n(" +   lessetB::formatNumber(std::format("{:< 10.5}",x),options,true,true)+ "; " + lessetB::formatNumber(std::format("{:> 10.5}",y),options,true,true) + ")";
}



void addEquation(std::vector<GraphEquation> &graphEquations, size_t &recalculateGraphIndex, std::pair<std::vector<std::vector<double>>,std::vector<std::vector<double>>> &graphsPoints)
{
    graphEquations.emplace_back();
    recalculateGraphIndex = graphEquations.size() - 1;
    graphsPoints.first.emplace_back(std::vector<double>());
    graphsPoints.second.emplace_back(std::vector<double>());
}

inline void displayStyleMenu(ImGuiStyle &style, lessetB::Options &options, bool &gayMode, bool &showGayMode, float &gayModeSpeed, float &hueModifier,
               float &saturationModifier)
{
    ImGui::Text("Save and Load styles in ini format.");
    if(!showGayMode) ImGui::SetNextItemWidth(300);
    if(ImGui::SliderFloat("##", &hueModifier, 0, 1, "Hue: %.3f", ImGuiSliderFlags_NoInput))
    {
        for(size_t i{6}; i < ImGuiCol_COUNT; i++)
        {
            if(i == ImGuiCol_MenuBarBg) continue;
            ImVec4 col{0, 0, 0, 1};
            ImGui::ColorConvertRGBtoHSV(style.Colors[i].x, style.Colors[i].y, style.Colors[i].z, col.x, col.y, col.z);
            col.x = hueModifier;
            ImGui::ColorConvertHSVtoRGB(col.x, col.y, col.z, style.Colors[i].x, style.Colors[i].y, style.Colors[i].z);
        }
    }

    if(!showGayMode) ImGui::SetNextItemWidth(300);
    if(ImGui::SliderFloat("##1", &saturationModifier, 0, 1, "Saturation: %.3f", ImGuiSliderFlags_NoInput))
    {
        for(size_t i{6}; i < ImGuiCol_COUNT; i++)
        {
            if(i == ImGuiCol_MenuBarBg) continue;
            ImVec4 col{0, 0, 0, 1};
            ImGui::ColorConvertRGBtoHSV(style.Colors[i].x, style.Colors[i].y, style.Colors[i].z, col.x, col.y, col.z);
            col.x = hueModifier;
            col.y = saturationModifier;

            ImGui::ColorConvertHSVtoRGB(col.x, col.y, col.z, style.Colors[i].x, style.Colors[i].y, style.Colors[i].z);
        }
    }
    if(ImGui::Button("Save Style"))
    {
        if(ImGui::IsKeyDown(ImGuiMod_Shift))
        {
            nfdu8char_t *outPath{};
            nfdsavedialogu8args_t args = {0};
            args.defaultName = "LessetStyle.ini";
            nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
            if(result == NFD_OKAY)
            {
                ImGui::SaveStylesTo(std::string(outPath).c_str());
                NFD_FreePathU8(outPath);
            }
        }
        else ImGui::SaveStylesTo("LessetStyle.ini");
    }
    ImGui::SetItemTooltip("Use shift to open a file dialog instead of saving to executable directory.");
    ImGui::SameLine();
    if(ImGui::Button("Load Style"))
    {
        if(std::filesystem::exists(std::string(std::string(std::filesystem::current_path()) + std::string("/LessetStyle.ini"))) &&
           !ImGui::IsKeyDown(ImGuiMod_Shift))
        {
            ImGui::LoadStyleFrom("LessetStyle.ini");
        }
        else
        {
            nfdu8char_t *outPath{};
            nfdopendialogu8args_t args = {0};
            nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
            if(result == NFD_OKAY)
            {
                ImGui::LoadStyleFrom(outPath);
                NFD_FreePathU8(outPath);
            }
        }
    }
    if(std::filesystem::exists(std::string(std::string(std::filesystem::current_path()) + std::string("/LessetStyle.ini"))))
    {
        ImGui::SetItemTooltip("Use shift to open a file dialog instead of loading from executable directory.");
    }
    else ImGui::SetItemTooltip("Save \"LessetStyle.ini\" to the executable's path to auto load it on startup.");

    if(showGayMode)
    {
        ImGui::SameLine();
        ImGui::Checkbox("Gay", &gayMode);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(98);
        ImGui::DragFloat("##2", &gayModeSpeed, 0.03, -10, 10, "Speed: %.2f", ImGuiSliderFlags_AlwaysClamp);
    }
    else ImGui::SameLine();
    if(ImGui::Checkbox("Decimal Comma", &lessetB::globals::useDecimalComma))
    {
        if(lessetB::globals::separator.at(0) == '.' && !lessetB::globals::useDecimalComma) lessetB::globals::separator.at(0) = ',';
        else if(lessetB::globals::separator.at(0) == ',' && lessetB::globals::useDecimalComma) lessetB::globals::separator.at(0) = '\'';
        writeSave(options);
    }
    ImGui::SetItemTooltip("Use comma for decimal places and semicolon for function argument separation. (Except in signatures)");

    ImGui::EndMenu();
}

void graphsMenuTopBarUI(bool &showGraphingMenuHint, std::vector<GraphEquation> &graphEquations, size_t &recalculateGraphIndex,
               std::pair<std::vector<std::vector<double>>, std::vector<std::vector<double>>> &graphsPoints, lessetB::Options &options,
               std::vector<Instance> &instances, bool &recalculateGraphs, size_t &currInst)
{
    showGraphingMenuHint = false;

    if(ImGui::Button("+", ImVec2(ROW_HEIGHT, ROW_HEIGHT)))
    {
        addEquation(graphEquations, recalculateGraphIndex, graphsPoints);
    }

    if(graphEquations.size() > 0)
    {
        ImGui::SameLine();
        if(ImGui::Button("×", ImVec2(ROW_HEIGHT, ROW_HEIGHT)))
        {
            resetGraphingData(graphEquations, graphsPoints);
        }

        ImGui::SameLine();
        if(ImGui::Button("Export"))
        {
            nfdu8char_t *outPath{};
            nfdsavedialogu8args_t args = {0};
            args.defaultName = "exportedEquations.ls";
            nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
            if(result == NFD_OKAY)
            {
                writeGraphsScript(outPath, graphEquations);
                NFD_FreePathU8(outPath);
            }
        }

        ImGui::SameLine();
        if(ImGui::Button("Save"))
        {
            writeGraphsScript(std::filesystem::path(), graphEquations, "savedEquations.ls");
        }
        ImGui::SameLine();
        if(std::filesystem::exists(std::string(std::string(std::filesystem::path()) + "savedEquations.ls")) && ImGui::Button("Load"))
        {
            std::string path = std::string(std::string(std::filesystem::path()) + "savedEquations.ls");
            bool hasRunScript = instances[currInst].hasRunScript;
            runScript(graphEquations, options, instances, recalculateGraphs, currInst, path, true, false);
            if(!hasRunScript) instances[currInst].hasRunScript = false;
        }
    }
    if(graphEquations.size() == 0)
    {
        graphEquations.emplace_back();
        graphEquations.at(0).col = ImGui::GetStyle().Colors[ImGuiCol_Button];
        graphEquations.at(0).col.w += 0.4;
        recalculateGraphIndex = graphEquations.size() - 1;
        graphsPoints.first.emplace_back(std::vector<double>());
        graphsPoints.second.emplace_back(std::vector<double>());
    }
    if(graphEquations.size() > 0)
    {
        ImGui::SameLine();
        ImGui::MenuItem("Equation Left, Function Signature Right.", NULL, false, false);
        ImGui::SetItemTooltip("You can use this menu to define custom functions.\nExample: Type b^2 into the equation, then f(b) into the "
                              "signature.\nThen use that function and see what happens!");
    }
}
#ifdef LESSET_INCLUDE_DEBUG
void debugMenu(std::vector<GraphEquation> &graphEquations, std::vector<Instance> &instances, size_t &currInst, const std::pair<std::vector<std::vector<double>>,std::vector<std::vector<double>>> &graphsPoints)
{
    const auto &io = ImGui::GetIO();
    ImGui::SliderInt("Graphing Precision##",&g_maxIndividualGraphPointsMultiplier,1,10);
    if(g_maxIndividualGraphPointsMultiplier<1) g_maxIndividualGraphPointsMultiplier=1;
    ImGui::Checkbox("Console Output", &lessetB::globals::debugCout);
    ImGui::Text("fps   : %d",static_cast<int>(1.0/io.DeltaTime));
    ImGui::Text("fpsavg: %d",static_cast<int>(io.Framerate+.5));
    size_t i{};
    for(auto &eq : graphEquations)
    {
        if(ImGui::BeginMenu(std::to_string(i).c_str()))
        {
            ImGui::InputText("eq", &eq.eq);
            ImGui::Separator();
            ImGui::InputText("fnName", &eq.fn.first);
            ImGui::InputText("fn.def", &eq.fn.second.definition);
            for(size_t j{}; j < eq.fn.second.argNames.size(); j++)
            {
                ImGui::Separator();
                ImGui::InputText(std::string("Arg " + std::to_string(j)).c_str(), &eq.fn.second.argNames.at(j));
            }
            ImGui::Separator();
            ImGui::Separator();
            ImGui::ColorEdit4(std::string("##c" + std::to_string(i)).c_str(), &eq.col.x, ImGuiColorEditFlags_NoInputs);
            auto timePerPoint = static_cast<int>(eq.timePerPoint.count());
            ImGui::SliderInt("Time/Point", &timePerPoint, 0, 100000);
            eq.timePerPoint = std::chrono::nanoseconds(timePerPoint);
            ImGui::Separator();
            ImGui::Text("%s",
                        std::string("Contains x: " +
                                    (containsX(eq.eq, instances[currInst].customSymbols) ? std::string("true") : std::string("false")))
                            .c_str());

            ImGui::Text("Points x: %zu",graphsPoints.first.at(i).size());
            ImGui::Text("Points y: %zu",graphsPoints.second.at(i).size());
            ImGui::EndMenu();
        }
        i++;
    }

    ImGui::EndMenu();
}
#endif

void updateSliders(size_t &timeStationary, std::vector<Instance> &instances, bool &recalculateGraphs, size_t &currInst)
{
    for(auto var : instances[currInst].customSymbols.m_variables)
    {
        if(instances[currInst].sliders.find(var.first) == instances[currInst].sliders.end()) break;
        if(instances[currInst].sliders.at(var.first).animate)
        {
            auto &sliders = instances[currInst].sliders;

            if(instances[currInst].sliders.at(var.first).nums[2] > instances[currInst].sliders.at(var.first).nums[1])
            {
                if(sliders.at(var.first).nums[0] > sliders.at(var.first).nums[2]) sliders.at(var.first).nums[0] = sliders.at(var.first).nums[2];
                if(sliders.at(var.first).nums[0] < sliders.at(var.first).nums[1]) sliders.at(var.first).nums[0] = sliders.at(var.first).nums[1];
            }
            else
            {
                if(sliders.at(var.first).nums[0] < sliders.at(var.first).nums[2]) sliders.at(var.first).nums[0] = sliders.at(var.first).nums[2];
                if(sliders.at(var.first).nums[0] > sliders.at(var.first).nums[1]) sliders.at(var.first).nums[0] = sliders.at(var.first).nums[1];
            }

            if(sliders.at(var.first).animMode != sliderAnimMode::TO_FRO_SMOOTH)
                sliders.at(var.first).nums[0] += (sliders.at(var.first).nums[2] - sliders.at(var.first).nums[1]) /
                                                 ((-1 + 2 * !sliders.at(var.first).movementDirection) * 500 * (1 / sliders.at(var.first).nums[3]));
            // dir*0.01+ (( dir*0.99 sin( π*normalize( sliderVal, max, min)))^2)^0.6 * ( (max-min)/(dir*200*(1/spd)) )
            // (Small constant speed + sine-wave thingy)*total_distance (distance being max-min)
            else
                sliders.at(var.first).nums[0] += pow(pow((-1 + 2 * !sliders.at(var.first).movementDirection) * 0.01 +
                                                             (-1 + 2 * !sliders.at(var.first).movementDirection) * 0.99 *
                                                                 sin(boost::math::constants::pi<double>() *
                                                                     lessetB::normalize(sliders.at(var.first).nums[0], sliders.at(var.first).nums[2],
                                                                                        sliders.at(var.first).nums[1])),
                                                         2),
                                                     .6) *
                                                 ((sliders.at(var.first).nums[2] - sliders.at(var.first).nums[1]) /
                                                  ((-1 + 2 * !sliders.at(var.first).movementDirection) * 200 * (1 / sliders.at(var.first).nums[3])));
            timeStationary = 0;
            lessetB::globals::tokenMemory.clear();
            if(sliders.at(var.first).nums[0] != sliders.at(var.first).nums[0])
                sliders.at(var.first).nums[0] = fmin(sliders.at(var.first).nums[2], sliders.at(var.first).nums[1]);
            switch(instances[currInst].sliders.at(var.first).animMode)
            {
                using enum sliderAnimMode;
            case TO_FRO:
            {
                if(instances[currInst].sliders.at(var.first).nums[2] > instances[currInst].sliders.at(var.first).nums[1])
                {
                    if(instances[currInst].sliders.at(var.first).nums[0] > instances[currInst].sliders.at(var.first).nums[2])
                        instances[currInst].sliders.at(var.first).movementDirection =
                            !instances[currInst].sliders.at(var.first).movementDirection;
                    if(instances[currInst].sliders.at(var.first).nums[0] < instances[currInst].sliders.at(var.first).nums[1])
                        instances[currInst].sliders.at(var.first).movementDirection =
                            !instances[currInst].sliders.at(var.first).movementDirection;
                }
                else
                {
                    if(instances[currInst].sliders.at(var.first).nums[0] < instances[currInst].sliders.at(var.first).nums[2])
                        instances[currInst].sliders.at(var.first).movementDirection =
                            !instances[currInst].sliders.at(var.first).movementDirection;
                    if(instances[currInst].sliders.at(var.first).nums[0] > instances[currInst].sliders.at(var.first).nums[1])
                        instances[currInst].sliders.at(var.first).movementDirection =
                            !instances[currInst].sliders.at(var.first).movementDirection;
                }
                break;
            }

            case TO_FRO_SMOOTH:
            {
                if(instances[currInst].sliders.at(var.first).nums[2] > instances[currInst].sliders.at(var.first).nums[1])
                {
                    if(instances[currInst].sliders.at(var.first).nums[0] > instances[currInst].sliders.at(var.first).nums[2])
                        instances[currInst].sliders.at(var.first).movementDirection = true;
                    if(instances[currInst].sliders.at(var.first).nums[0] < instances[currInst].sliders.at(var.first).nums[1])
                        instances[currInst].sliders.at(var.first).movementDirection = false;
                }
                else
                {
                    if(instances[currInst].sliders.at(var.first).nums[0] < instances[currInst].sliders.at(var.first).nums[2])
                        instances[currInst].sliders.at(var.first).movementDirection = true;
                    if(instances[currInst].sliders.at(var.first).nums[0] > instances[currInst].sliders.at(var.first).nums[1])
                        instances[currInst].sliders.at(var.first).movementDirection = false;
                }
                break;
            }

            case TO:
            {
                if(instances[currInst].sliders.at(var.first).movementDirection)
                    instances[currInst].sliders.at(var.first).movementDirection = false;
                if(instances[currInst].sliders.at(var.first).nums[2] > instances[currInst].sliders.at(var.first).nums[1])
                {
                    if(instances[currInst].sliders.at(var.first).nums[0] > instances[currInst].sliders.at(var.first).nums[2])
                        instances[currInst].sliders.at(var.first).nums[0] = instances[currInst].sliders.at(var.first).nums[1];
                    if(instances[currInst].sliders.at(var.first).nums[0] < instances[currInst].sliders.at(var.first).nums[1])
                        instances[currInst].sliders.at(var.first).nums[0] = instances[currInst].sliders.at(var.first).nums[1];
                }
                else
                {
                    if(instances[currInst].sliders.at(var.first).nums[0] < instances[currInst].sliders.at(var.first).nums[2])
                        instances[currInst].sliders.at(var.first).nums[0] = instances[currInst].sliders.at(var.first).nums[1];
                    if(instances[currInst].sliders.at(var.first).nums[0] > instances[currInst].sliders.at(var.first).nums[1])
                        instances[currInst].sliders.at(var.first).nums[0] = instances[currInst].sliders.at(var.first).nums[2];
                }
                break;
            }

            default:
            {
                if(instances[currInst].sliders.at(var.first).nums[2] > instances[currInst].sliders.at(var.first).nums[1])
                {
                    if(instances[currInst].sliders.at(var.first).nums[0] > instances[currInst].sliders.at(var.first).nums[2])
                        instances[currInst].sliders.at(var.first).nums[0] = instances[currInst].sliders.at(var.first).nums[1];
                    if(instances[currInst].sliders.at(var.first).nums[0] < instances[currInst].sliders.at(var.first).nums[1])
                        instances[currInst].sliders.at(var.first).nums[0] = instances[currInst].sliders.at(var.first).nums[1];
                }
                else
                {
                    if(instances[currInst].sliders.at(var.first).nums[0] < instances[currInst].sliders.at(var.first).nums[2])
                        instances[currInst].sliders.at(var.first).nums[0] = instances[currInst].sliders.at(var.first).nums[1];
                    if(instances[currInst].sliders.at(var.first).nums[0] > instances[currInst].sliders.at(var.first).nums[1])
                        instances[currInst].sliders.at(var.first).nums[0] = instances[currInst].sliders.at(var.first).nums[2];
                }
            }
            }
            addVariable(std::pair<std::string, std::string>(var.first, std::to_string(instances[currInst].sliders.at(var.first).nums[0])),
                        instances[currInst]);
            recalculateGraphs = true;
        }
    }
}

bool hasNAN(const std::pair<std::vector<std::vector<double>>, const std::vector<std::vector<double>>> &graphsPoints, const size_t j, const size_t increment, const size_t i)
{
    for(size_t k{i}; k < i + increment; k++)
    {
        if(graphsPoints.second.at(j).at(k) != graphsPoints.second.at(j).at(k))
        {
            return true;
        }
    }
    return false;
}

double distanceBetweenTwo(double x1, double y1, double x2, double y2)
{
    return sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
}

template<typename T>
double distanceBetweenTwo(T beg, T end)
{
    return sqrt((end.x-beg.x)*(end.x-beg.x)+(end.y-beg.y)*(end.y-beg.y));
}

ImPlotPoint markPoints(const std::vector<GraphEquation> &graphEquations, const size_t timeStationary,
               const std::pair<std::vector<std::vector<double>>, std::vector<std::vector<double>>> &graphsPoints, const lessetB::Options &options,
               const ImPlotRect &limits, const size_t eqIndex, ImPlotSpec &pointsSpec, bool &hasShownMousePointText, SpecialPoint &closestSpecialPointToCursor)
{
    ImPlotPoint mousePointTextLocation(NAN,NAN);
    bool textAbove{};
    bool hasShownPoint{false};
    double xPreviousPointMarked{-INFINITY};
    size_t increment = 4/(fmax(1,graphEquations[eqIndex].timePerPoint.count()/16000));
    if(increment<1) increment=1;
    pointsSpec.MarkerFillColor=graphEquations[eqIndex].col;
    pointsSpec.MarkerLineColor=graphEquations[eqIndex].col;
    ImVec4 col;
    ImGui::ColorConvertRGBtoHSV(graphEquations[eqIndex].col.x,graphEquations[eqIndex].col.y,graphEquations[eqIndex].col.z, col.x, col.y, col.z);
    if(col.y>.5) col.y-=.2;
    else col.y+=.2;
    col.z=1;
    col.w=1;
    ImGui::ColorConvertHSVtoRGB(col.x, col.y, col.z, col.x, col.y, col.z);
    ImPlot::PushStyleColor(ImPlotCol_InlayText,col);
    if(timeStationary >= HIGH_PRECISION_DRAW_DELAY) increment = 5;

    for(size_t i{increment}; i < graphsPoints.first.at(eqIndex).size() - 20; i += increment)
    {
        // Mouse point
        ImPlotPoint mousePos = ImPlot::GetPlotMousePos();
        if(abs(mousePos.x - graphsPoints.first.at(eqIndex).at(i)) <
               abs(mousePos.x - graphsPoints.first.at(eqIndex).at(i + increment)) &&
           abs(mousePos.x - graphsPoints.first.at(eqIndex).at(i)) <
               abs(mousePos.x - graphsPoints.first.at(eqIndex).at(i - increment)) &&
           !hasShownPoint)
        {
            hasShownPoint = true;
            if(abs(mousePos.y - graphsPoints.second.at(eqIndex).at(i)) < (limits.Y.Max - limits.Y.Min) / 5)
            {
                
                ImPlot::PlotScatter("##", &graphsPoints.second.at(eqIndex).at(i), 1, 0, graphsPoints.first.at(eqIndex).at(i), pointsSpec);
                if(abs(mousePos.y - graphsPoints.second.at(eqIndex).at(i)) < (limits.Y.Max - limits.Y.Min) / 20 &&
                    !hasShownMousePointText)
                {
                    mousePointTextLocation=ImPlotPoint(graphsPoints.first.at(eqIndex).at(i),graphsPoints.second.at(eqIndex).at(i)-ImPlot::PixelsToPlot(ImVec2(0, 60)).y);
                    hasShownMousePointText = true;
                    std::string coordsFormatted = formatCoords(graphEquations[eqIndex], graphsPoints.first.at(eqIndex).at(i), graphsPoints.second.at(eqIndex).at(i), options);
                    ImPlot::PlotText(coordsFormatted.c_str(), graphsPoints.first.at(eqIndex).at(i), graphsPoints.second.at(eqIndex).at(i), ImVec2(0, 30));
                }
            }
        }

        // Extremes and zeroes

        if(( // (graphsPoints.first.at(j).at(i)<0 && graphsPoints.first.at(j).at(i+increment)>0) ||
                (graphsPoints.second.at(eqIndex).at(i) < graphsPoints.second.at(eqIndex).at(i + increment) &&
                graphsPoints.second.at(eqIndex).at(i) < graphsPoints.second.at(eqIndex).at(i - increment)) ||
                (graphsPoints.second.at(eqIndex).at(i) > graphsPoints.second.at(eqIndex).at(i + increment) &&
                graphsPoints.second.at(eqIndex).at(i) > graphsPoints.second.at(eqIndex).at(i - increment))) &&
            (abs(graphsPoints.first.at(eqIndex).at(i) - xPreviousPointMarked) > abs(limits.X.Max - limits.X.Min) / 50))
        {
            if(!hasNAN(graphsPoints, eqIndex, increment, i))
            {
                xPreviousPointMarked = graphsPoints.first.at(eqIndex).at(i);
                ImPlot::PlotScatter("##", &graphsPoints.second.at(eqIndex).at(i), 1, 0, graphsPoints.first.at(eqIndex).at(i),pointsSpec);

                if(abs(mousePos.x - graphsPoints.first.at(eqIndex).at(i)) < (limits.X.Max - limits.X.Min) / 10 &&
                    abs(mousePos.y - graphsPoints.second.at(eqIndex).at(i)) < (limits.Y.Max - limits.Y.Min) / 10)
                {
                    std::string coordsFormatted = formatCoords(graphEquations[eqIndex], graphsPoints.first.at(eqIndex).at(i), graphsPoints.second.at(eqIndex).at(i), options);
                    
                    double distance = distanceBetweenTwo(ImPlotPoint(graphsPoints.first[eqIndex][i],graphsPoints.second[eqIndex][i]), mousePos);
                    double oldDistance = distanceBetweenTwo(ImPlotPoint(closestSpecialPointToCursor.x,closestSpecialPointToCursor.y), mousePos);
                    assert((oldDistance>=0 || oldDistance!=oldDistance) && distance>=0);
                    if(oldDistance!=oldDistance || distance<oldDistance)
                    {
                        closestSpecialPointToCursor.eq=graphEquations[eqIndex].eq;
                        closestSpecialPointToCursor.col=col;
                        closestSpecialPointToCursor.x=graphsPoints.first[eqIndex][i];
                        closestSpecialPointToCursor.y=graphsPoints.second[eqIndex][i];
                    }
                    
                }
            }
        }
        else if(((graphsPoints.second.at(eqIndex).at(i) > 0 && graphsPoints.second.at(eqIndex).at(i + increment) <= 0) ||
                    (graphsPoints.second.at(eqIndex).at(i) < 0 && graphsPoints.second.at(eqIndex).at(i + increment) >= 0)) &&
                (abs(graphsPoints.first.at(eqIndex).at(i) - xPreviousPointMarked) > abs(limits.X.Max - limits.X.Min) / 50))
        {
            if(!hasNAN(graphsPoints, eqIndex, increment, i))
            {
                xPreviousPointMarked = graphsPoints.first.at(eqIndex).at(i);
                const float zero = 0;
                ImPlot::PlotScatter("##", &zero, 1, 0, graphsPoints.first.at(eqIndex).at(i), pointsSpec);

                if(abs(mousePos.x - graphsPoints.first.at(eqIndex).at(i)) < (limits.X.Max - limits.X.Min) / 10 &&
                    abs(mousePos.y - graphsPoints.second.at(eqIndex).at(i)) < (limits.Y.Max - limits.Y.Min) / 10)
                {
                    std::string coordsFormatted = formatCoords(graphEquations[eqIndex], graphsPoints.first.at(eqIndex).at(i), zero, options);
                    double distance = distanceBetweenTwo(ImPlotPoint(graphsPoints.first[eqIndex][i],graphsPoints.second[eqIndex][i]), mousePos);
                    double oldDistance = distanceBetweenTwo(ImPlotPoint(closestSpecialPointToCursor.x,closestSpecialPointToCursor.y), mousePos);
                    assert((oldDistance>=0 || oldDistance!=oldDistance) && distance>=0);
                    if(oldDistance!=oldDistance || distance<oldDistance)
                    {
                        closestSpecialPointToCursor.eq=graphEquations[eqIndex].eq;
                        closestSpecialPointToCursor.col=col;
                        closestSpecialPointToCursor.x=graphsPoints.first[eqIndex][i];
                        closestSpecialPointToCursor.y=0;
                    }
                }
            }
        }
    }
    ImPlot::PushStyleColor(ImPlotCol_InlayText,ImVec4(1,1,1,1));
    return mousePointTextLocation;
}

void setupPlot(const std::array<float, 2> &defaultBoundsX, const std::array<float, 2> &defaultBoundsY)
{
    ImPlotSpec specHidden;
    specHidden.FillAlpha=0;
    specHidden.LineColor=ImVec4(0,0,0,0);
    specHidden.Flags=ImPlotColormapScaleFlags_NoLabel;
    ImPlot::SetupAxes("", "", ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoGridLines|ImPlotAxisFlags_NoSideSwitch, ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoGridLines|ImPlotAxisFlags_NoSideSwitch);
    ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Horizontal);
    ImPlot::PlotLine("", defaultBoundsX.data(), defaultBoundsY.data(), 2, specHidden);
}

int inputFormatter(GraphEquation &graphEquation, const Instance &instance)
{
    int unclosed = addClosingParentheses(graphEquation.eq);
    replaceWithUnicode(graphEquation.eq, lessetB::globals::symbols, lessetB::globals::multiArgFunctions, instance.customSymbols);
    addSeparatorsAfterFunctions(graphEquation.eq, instance, " ");
    cleanWhitespace(graphEquation.eq);
    return unclosed;
}

int inputFormatter(std::string &eq, const Instance &instance)
{
    int unclosed = addClosingParentheses(eq);
    replaceWithUnicode(eq, lessetB::globals::symbols, lessetB::globals::multiArgFunctions, instance.customSymbols);
    addSeparatorsAfterFunctions(eq, instance, " ");
    cleanWhitespace(eq);
    return unclosed;
}
