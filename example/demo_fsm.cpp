#include <string>
#include <iostream>
#include "tinyfsm.h"

enum OjbectForm {
    solid,
    liquid,
    gas
};

enum ObjectEvent {
    melt,
    freeze,
    vaporize,
    condense,

    timer_1,
    timer_2,
};

int main(int argc, const char *argv[]) {

    tiny::TimerFSM fsm(solid);
    fsm.addTransition(solid, liquid, melt);
    fsm.addTransition(liquid, gas, vaporize);
    fsm.addTransition(gas, liquid, condense);
    fsm.addTransition(liquid, solid, freeze);

    fsm.onEnter(liquid, [](){
        std::cout << "enter state: liquid" << std::endl;
    });

    fsm.onLeave(liquid, [](){
        std::cout << "leave state: liquid" << std::endl;
    });

    fsm.onBefore(melt, [](){
        std::cout << "before: melt" << std::endl;
    });

    fsm.onAfter(melt, [](){
        std::cout << "after: melt" << std::endl;
    });

    fsm.onTransition([](uint32_t from, uint32_t to, uint32_t event){
        std::cout << from << "--" << event << "--->" << to << std::endl;
    });

    while (1) {
        char c;
        std::cout << "m=melt, f=freeze, v=vaporize, c=condense, q=quit ? " << std::endl;
        std::cout << ">";
        std::cin >> c;
        switch (c) {
            case 'm':
                fsm.transit(melt);
                break;
            case 'f':
                fsm.transit(freeze);
                break;
            case 'v':
                fsm.transit(vaporize);
                break;
            case 'c':
                fsm.transit(condense);
                break;
            case 'q':
                std::cout << "Thanks for playing!" << std::endl;
                return 0;
            default:
                std::cout << "Invalid input" << std::endl;
        };
    }

    return 0;
}
