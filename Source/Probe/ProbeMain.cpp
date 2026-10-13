#include "Analysis/SliceAnalysis.h"

#include <iostream>

int main(int argumentCount, char* arguments[])
{
    SliceAnalysis         analysis;
    SliceAnalysis::Result result;

    if (argumentCount < 2) {
        std::cout << "usage: CatKitProbe <audio file>\n";
        return 1;
    }

    result = analysis.analyse(juce::File::getCurrentWorkingDirectory().getChildFile(arguments[1]));

    std::cout << SliceAnalysis::summarise(result);

    if (result.error.isNotEmpty()) {
        return 2;
    }

    return 0;
}
