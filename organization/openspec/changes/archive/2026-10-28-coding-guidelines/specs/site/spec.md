## MODIFIED Requirements

### Requirement: Site: The community page and the search page
The page `community/` SHALL link CONTRIBUTING.md, CODING-GUIDELINES.md (the rules and the design decisions), AGENTS.md (how to work in the tree) and the section "Bringing a library" on `https://github.com/webcpporg/webcpp/blob/main/` and the organization's issues, saying a library joins webcpp through an issue that proposes it. The page `search/` SHALL hold a form with `action="https://www.google.com/search"` and `method="get"` whose first `q` is a hidden `site:<channel host>` (`site:webcpporg.github.io` on the preview) and whose second is the reader's own search box (`required`, `autocomplete` off), so that Google joins the two into one query; opened as `search/?q=<terms>`, the JSON-LD's SearchAction, the box starts with those terms. The Programmable Search element replaces the form once the owner creates its engine.

#### Scenario: The search form on the preview
- **WHEN** the search page renders on the preview
- **THEN** its form posts to Google with `<input type="hidden" name="q" value="site:webcpporg.github.io">` and a search input named `q`

#### Scenario: The rules linked from the community page
- **WHEN** the community page renders
- **THEN** it links `https://github.com/webcpporg/webcpp/blob/main/CODING-GUIDELINES.md` as the rules, beside `CONTRIBUTING.md` and `AGENTS.md`
