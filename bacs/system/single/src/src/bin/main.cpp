#include "bacs/single/api/pb/task.pb.h"

#include "yandex/contest/TypeInfo.hpp"
#include "yandex/contest/system/Trace.hpp"

#include <iostream>

#include <boost/assert.hpp>

using namespace bacs::single;

int main(int argc, char *argv[])
{
    try
    {
        yandex::contest::system::Trace::handle(SIGABRT);
        BOOST_ASSERT(argc == 1);
        (void) argv;
        api::pb::task::Task task;
        task.ParseFromIstream(&std::cin);
    }
    catch (std::exception &e)
    {
        std::cerr << "Program terminated due to exception of type \"" <<
                     yandex::contest::typeinfo::name(e) << "\"." << std::endl;
        std::cerr << "what() returns the following message:" << std::endl <<
                     e.what() << std::endl;
        return 1;
    }
}