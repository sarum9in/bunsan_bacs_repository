#include "bacs/single/tests.hpp"

#include <memory>

#include <boost/iterator/filter_iterator.hpp>
#include <boost/regex.hpp>

#include <fnmatch.h>

namespace bacs{namespace single
{
    namespace
    {
        class matcher
        {
        public:
            template <typename T>
            explicit matcher(const T &query):
                m_impl(impl_(query)) {}

            matcher(const matcher &)=default;
            matcher &operator=(const matcher &)=default;

            bool operator()(const std::string &test_id) const
            {
                BOOST_ASSERT(m_impl);
                return m_impl->match(test_id);
            }

        private:
            class impl
            {
            public:
                virtual ~impl() {}

                virtual bool match(const std::string &test_id) const=0;
            };

            class id: public impl
            {
            public:
                explicit id(const std::string &test_id): m_test_id(test_id) {}

                bool match(const std::string &test_id) const override
                {
                    return test_id == m_test_id;
                }

            private:
                const std::string m_test_id;
            };

            class wildcard: public impl
            {
            public:
                explicit wildcard(const api::pb::testing::WildcardQuery &query):
                    m_wildcard(query.value()), m_flags(flags(query)) {}

                bool match(const std::string &test_id) const override
                {
                    return ::fnmatch(m_wildcard.c_str(), test_id.c_str(), m_flags);
                }

            private:
                int flags(const api::pb::testing::WildcardQuery &query)
                {
                    int flags_ = 0;
                    for (const int flag: query.flags())
                    {
                        switch (static_cast<api::pb::testing::WildcardQuery::Flag>(flag))
                        {
                        case api::pb::testing::WildcardQuery::IGNORE_CASE:
                            flags_ |= FNM_CASEFOLD;
                            break;
                        }
                    }
                    return flags_;
                }

            private:
                const std::string m_wildcard;
                int m_flags;
            };

            class regex: public impl
            {
            public:
                explicit regex(const api::pb::testing::RegexQuery &query):
                    m_regex(query.value(), flags(query)) {}

                bool match(const std::string &test_id) const override
                {
                    return boost::regex_match(test_id, m_regex);
                }

            private:
                boost::regex_constants::syntax_option_type flags(
                    const api::pb::testing::RegexQuery &query)
                {
                    boost::regex_constants::syntax_option_type flags_ = boost::regex_constants::normal;
                    for (const int flag: query.flags())
                    {
                        switch (static_cast<api::pb::testing::RegexQuery::Flag>(flag))
                        {
                        case api::pb::testing::RegexQuery::IGNORE_CASE:
                            flags_ |= boost::regex_constants::icase;
                            break;
                        }
                    }
                    return flags_;
                }

            private:
                const boost::regex m_regex;
            };

            class any: public impl, public std::vector<std::unique_ptr<impl>>
            {
            public:
                bool match(const std::string &test_id) const override
                {
                    for (const std::unique_ptr<impl> &imp: *this)
                        if (imp->match(test_id))
                            return true;
                    return false;
                }
            };

        private:
            static std::unique_ptr<impl> impl_(
                const google::protobuf::RepeatedPtrField<api::pb::testing::TestQuery> &test_query)
            {
                std::unique_ptr<any> tmp;
                for (const api::pb::testing::TestQuery &query: test_query)
                    tmp->push_back(impl_(query));
                return std::move(tmp);
            }

            static std::unique_ptr<impl> impl_(const api::pb::testing::TestQuery &query)
            {
                std::unique_ptr<impl> tmp;
                if (query.has_id())
                    tmp.reset(new id(query.id()));
                else if (query.has_wildcard())
                    tmp.reset(new wildcard(query.wildcard()));
                else if (query.has_regex())
                    tmp.reset(new regex(query.regex()));
                else
                    BOOST_THROW_EXCEPTION(invalid_test_query_error());
                return std::move(tmp);
            }

        private:
            std::shared_ptr<impl> m_impl;
        };
    }

    std::unordered_set<std::string> tests::test_set(
        const google::protobuf::RepeatedPtrField<api::pb::testing::TestQuery> &test_query)
    {
        const matcher m(test_query);
        const std::unordered_set<std::string> full_set = test_set();
        return std::unordered_set<std::string>{
            boost::make_filter_iterator(m, full_set.begin()),
            boost::make_filter_iterator(m, full_set.end())
        };
    }
}}
