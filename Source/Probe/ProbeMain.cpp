#include "Analysis/CorpusAnalysis.h"

#include <iostream>

int main(int argumentCount, char* arguments[])
{
    CorpusAnalysis         analysis;
    CorpusAnalysis::Result result;

    if (argumentCount < 2) {
        std::cout << "usage: CatKitProbe <audio file or folder>\n";
        return 1;
    }

    result = analysis.analyse(juce::File::getCurrentWorkingDirectory().getChildFile(arguments[1]));

    std::cout << CorpusAnalysis::summarise(result);

    if (result.error.isNotEmpty()) {
        return 2;
    }

    return 0;
}
