
/* yaml/Parser.h
	Header for yaml::Parser. */

#pragma once

#include <yaml.h>

#include <exception>

#include "Node.h"
#include "YamlExn.h"

namespace yaml
{
	/// Standard exception thrown for parser errors.
	class ParserExn : public YamlExn
	{
		typedef YamlExn SUPER;

		public:
			ParserExn() : SUPER() { }
			ParserExn(const char *const &s) : SUPER(s) { }
			virtual ~ParserExn() throw() { }
	};

	/// Exception for when the document is empty or missing the header.
	class EmptyDocParserExn : public ParserExn
	{
		typedef ParserExn SUPER;

		public:
			EmptyDocParserExn() : SUPER() { }
			EmptyDocParserExn(const char *const &s) : SUPER(s) { }
			virtual ~EmptyDocParserExn() throw() { }
	};

	/// Wrapper for the LibYAML parser.
	class Parser
	{
		private:
			Parser() { }
		public:
			Parser(FILE *file);
			virtual ~Parser();

		private:
			void Cleanup();

		public:
			Node *GetRootNode() const { return root; }

		private:
			yaml_parser_t parser;
			yaml_document_t *doc;
			Node *root;
	};
}
