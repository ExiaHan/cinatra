#include <boost/test/unit_test.hpp>
#include <cinatra/request_parser.hpp>

using namespace cinatra;

BOOST_AUTO_TEST_CASE(http_parser_simple_test)
{
	RequestParser parser;
	std::string content = "GET /test HTTP/1.1\r\n";
	BOOST_REQUIRE(parser.parse(content.begin(), content.end()) != RequestParser::bad);
	content = "\r\n";
	BOOST_REQUIRE(parser.parse(content.begin(), content.end()) == RequestParser::good);
	auto req = parser.get_request();
	BOOST_CHECK(req.raw_url == "/test");
	BOOST_CHECK(req.raw_body.size() == 0);
	BOOST_CHECK(req.method == "GET");
	BOOST_CHECK(req.path == "/test");
}

BOOST_AUTO_TEST_CASE(http_parser_header_test)
{
	RequestParser parser;
	std::string content = "POST /test HTTP/1.1\r\n";
	BOOST_REQUIRE(parser.parse(content.begin(), content.end()) != RequestParser::bad);
	
	/** FIXME: 测试失败，key:val -> key: val则成功 */
	content = "key1:val1\r\nkey1: val11\r\nkey2: val2\r\n\r\n";
	BOOST_REQUIRE(parser.parse(content.begin(), content.end()) == RequestParser::good);
	auto req = parser.get_request();
	BOOST_CHECK(req.raw_url == "/test");
	BOOST_CHECK(req.raw_body.size() == 0);
	BOOST_CHECK(req.method == "POST");
	BOOST_CHECK(req.path == "/test");
	auto header = req.header;
	BOOST_CHECK(header.get_count("key1") == 2);
	BOOST_CHECK(header.get_count("key2") == 1);
	BOOST_CHECK(header.get_val("key1") == "val1" || header.get_val("key1") == "val11");
	BOOST_CHECK(header.get_val("key2") == "val2");
	auto r = header.get_vals("key1");
	BOOST_REQUIRE(r.size() == 2);
	BOOST_CHECK((r[0] == "val1" && r[1] == "val11") || (r[0] == "val11" && r[1] == "val1"));	
}

BOOST_AUTO_TEST_CASE(http_parser_body_test)
{
	RequestParser parser;
	std::string content = "PUT /test HTTP/1.1\r\n";
	BOOST_REQUIRE(parser.parse(content.begin(), content.end()) != RequestParser::bad);
	content = "Content-Length: 4\r\n\r\nbody";
	BOOST_REQUIRE(parser.parse(content.begin(), content.end()) == RequestParser::good);
	auto req = parser.get_request();
	BOOST_CHECK(std::string(req.raw_body.begin(), req.raw_body.end()) == "body");
	BOOST_CHECK(req.body.size() == 0);
}

BOOST_AUTO_TEST_CASE(http_parser_kv_body_test)
{
	RequestParser parser;
	std::string content = "POST /test HTTP/1.1\r\n";
	BOOST_REQUIRE(parser.parse(content.begin(), content.end()) != RequestParser::bad);
	content = "Content-Length: 3\r\n\r\nk=v";
	BOOST_REQUIRE(parser.parse(content.begin(), content.end()) == RequestParser::good);
	auto req = parser.get_request();
	BOOST_CHECK(std::string(req.raw_body.begin(), req.raw_body.end()) == "k=v");
	BOOST_REQUIRE(req.body.has_key("k"));
	BOOST_CHECK(req.body.get_val("k") == "v");
}

BOOST_AUTO_TEST_CASE(http_parser_map_test)
{
	RequestParser p;
	std::string content = "GET /test?k1=v1&k2=v2&k3=v3 HTTP/1.1\r\nke1: va1\r\nke2: va2\r\n"
		"Content-Length: 17\r\n\r\nkey1=val1&key2=val2&key3=val3";
	const size_t size = content.size();

	/** FIXME: 测试失败 */
	size_t pos = 0;
	for(; pos < size - 1; ++pos)
		BOOST_CHECK(p.parse(content.begin() + pos, content.begin() + pos + 1) == RequestParser::indeterminate);
	BOOST_CHECK(p.parse(content.begin() + pos, content.begin() + pos + 1) == RequestParser::good);

	auto req = p.get_request();
	BOOST_CHECK(req.method == "GET");
	BOOST_CHECK(req.path == "/test");
	BOOST_CHECK(req.raw_url == "/test?k1=v1&k2=v2&k3=v3");

	BOOST_CHECK(req.query.get_val("k1") == "v1");
	BOOST_CHECK(req.query.get_val("k2") == "v2");
	BOOST_CHECK(req.query.get_val("k3") == "v3");

	BOOST_CHECK(req.header.get_val("ke1") == "va1");
	BOOST_CHECK(req.header.get_val("ke2") == "va2");
	BOOST_CHECK(req.header.get_val("ke3") == "va3");
	
	BOOST_CHECK(req.body.get_val("key1") == "val1");
	BOOST_CHECK(req.body.get_val("key2") == "val2");
	BOOST_CHECK(req.body.get_val("key3") == "val3");
}
