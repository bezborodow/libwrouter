import wrouter

cases = [
    ("/downloads/*", "/downloads/documents/schematic.pdf", None),
    ("/downloads/", "/downloads/", None),
    ("/", "/", None),
    ("/*", "/hello", None),
    ("/accounts", "/accounts", None),
    ("/accounts/create", "/accounts/create", None),

    ("/account/<account_id>", "/account/100", {"account_id": "100"}),
    ("/account/<account_id>/edit", "/account/100/edit", {"account_id": "100"}),
    ("/account/<account_id>/projects", "/account/100/projects", {"account_id": "100"}),
    ("/account/<account_id>/contacts", "/account/100/contacts", {"account_id": "100"}),

    (
        "/account/<account_id>/contact/<account_contact_id>",
        "/account/200/contact/300",
        {"account_id": "200", "account_contact_id": "300"}
    ),

    (
        "/account/<account_id>/contact/<account_contact_id>/credentials/*",
        "/account/200/contact/300/credentials/letter_of_endorsement.pdf",
        {"account_id": "200", "account_contact_id": "300"}
    ),

    ("/projects", "/projects", None),
    ("/projects/create", "/projects/create", None),

    ("/project/<project_id>", "/project/400", {"project_id": "400"}),
    ("/project/<project_id>/edit", "/project/400/edit", {"project_id": "400"}),
]

builder = wrouter.Builder(param_syntax = wrouter.ANGLE)

# register routes
for pattern, _, _ in cases:
    builder.add(pattern, pattern)  # ctx = pattern string

router = builder.compile()
dispatcher = wrouter.Dispatcher(router)
#dispatcher = router.dispatcher()

# resolve + basic checks
for pattern, request, expected_params in cases:
    ctx = dispatcher.resolve(request)

    print(f"{ctx} {pattern}")
    assert ctx == pattern

# no direct params API in current Python binding, so only ctx is verifiable
print("OK")
