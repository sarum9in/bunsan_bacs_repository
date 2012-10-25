#include "bunsan/config.hpp"
#include "bunsan/system_error.hpp"

#include "yandex/contest/StreamEnum.hpp"

#include <unordered_set>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/assert.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include "yandex/contest/serialization/unordered_set.hpp"

namespace
{
    struct newline_error: virtual bunsan::error {};
    struct not_cr_eoln_in_cr_file: virtual newline_error {};
    struct not_lf_eoln_in_lf_file: virtual newline_error {};
    struct not_crlf_eoln_in_crlf_file: virtual newline_error {};


    YANDEX_CONTEST_STREAM_ENUM(eoln,
    (
        CR_,  ///< CR, may be CRLF
        CR,   ///< definitely CR
        LF,   ///< definitely LF
        CRLF, ///< definitely CRLF
        NA    ///< not available
    ))

    bool is_ordinary(char c)
    {
        return c != '\n' && c != '\r';
    }

    eoln transform(const boost::filesystem::path &src, const boost::filesystem::path &dst)
    {
        boost::filesystem::ifstream fin(src, std::ios_base::binary);
        if (!fin.is_open())
            BOOST_THROW_EXCEPTION(bunsan::system_error("open file: \"" + src.string() + "\""));
        boost::filesystem::ofstream fout(dst, std::ios_base::binary);
        if (!fout.is_open())
            BOOST_THROW_EXCEPTION(bunsan::system_error("open file: \"" + dst.string() + "\""));
        eoln state = NA;
        char c, p;
        while (fin.get(c) && fout)
        {
            bool ord = is_ordinary(c);
            switch (state)
            {
            case CR_:
                if (ord || c == '\r')
                {
                    state = CR;
                    fout.put('\n');
                    if (c == '\r')
                        fout.put('\n');
                    else
                        fout.put(c);
                }
                else
                {
                    BOOST_ASSERT(c == '\n');
                    state = CRLF;
                    fout.put('\n');
                }
                break;
            case CR:
                if (ord)
                    fout.put(c);
                else if (c == '\r')
                    fout.put('\n');
                else
                    BOOST_THROW_EXCEPTION(not_cr_eoln_in_cr_file());
                break;
            case LF:
                if (ord || c == '\n')
                    fout.put(c);
                else
                    BOOST_THROW_EXCEPTION(not_lf_eoln_in_lf_file());
                break;
            case CRLF:
                if (ord)
                    fout.put(c);
                else if (c == '\n')
                {
                    if (p != '\r')
                        BOOST_THROW_EXCEPTION(not_crlf_eoln_in_crlf_file());
                    fout.put('\n');
                }
                // we will skip \r
                break;
            case NA:
                if (ord)
                    fout.put(c);
                else
                    switch (c)
                    {
                    case '\r':
                        state = CR_;
                        break;
                    case '\n':
                        state = LF;
                        fout.put('\n');
                        break;
                    }
                break;
            }
            p = c;
        }
        if (fin.bad())
            BOOST_THROW_EXCEPTION(bunsan::system_error("reading from file: \"" + src.string() + "\""));
        if (fout.bad())
            BOOST_THROW_EXCEPTION(bunsan::system_error("writing to file: \"" + dst.string() + "\""));
        return state;
    }
}

int main(int argc, char *argv[])
{
    BOOST_ASSERT(argc >= 2 + 1);
    std::unordered_set<std::string> test_set, data_set, text_data_set;
    {
        boost::filesystem::ifstream fin("etc/tests");
        if (fin.bad())
            BOOST_THROW_EXCEPTION(bunsan::system_error("open"));
        {
            boost::archive::text_iarchive ia(fin);
            ia >> test_set >> data_set >> text_data_set;
        }
        if (fin.bad())
            BOOST_THROW_EXCEPTION(bunsan::system_error("read"));
        if (fin.bad())
            BOOST_THROW_EXCEPTION(bunsan::system_error("close"));
    }
    {
        boost::filesystem::ofstream fout(argv[1]);
        if (fout.bad())
            BOOST_THROW_EXCEPTION(bunsan::system_error("open"));
        {
            boost::archive::text_oarchive oa(fout);
            oa << test_set << data_set;
        }
        if (fout.bad())
            BOOST_THROW_EXCEPTION(bunsan::system_error("write"));
        if (fout.bad())
            BOOST_THROW_EXCEPTION(bunsan::system_error("close"));
    }
    const boost::filesystem::path dst_dir = argv[2];
    for (int i = 3; i < argc; ++i)
    {
        const boost::filesystem::path test = argv[i];
        const boost::filesystem::path dst = dst_dir / test.filename();
        const std::string data_id = test.extension().string();
        BOOST_ASSERT(!data_id.empty());
        std::cerr << test.filename() << ": ";
        if (text_data_set.find(data_id.substr(1)) != text_data_set.end())
        {
            const eoln eoln_ = transform(test, dst);
            std::cerr << eoln_;
        }
        else
        {
            boost::filesystem::copy(test, dst);
            std::cerr << "binary";
        }
        std::cerr << std::endl;
    }
}
