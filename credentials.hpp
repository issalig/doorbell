const char* mdnsName = "doorbell"; // Domain name for the mDNS responder
const char *OTAName = "doorbell";           // A name and a password for the OTA service
const char *OTAPassword = "";


String ssid  = ""    ; // REPLACE ssid WITH YOUR WIFI SSID
String pass  = ""; // REPLACE pass YOUR WIFI PASSWORD, IF ANY

String token = ""   ; // REPLACE token WITH YOUR TELEGRAM BOT TOKEN
int32_t chat_id = 0; // REPLACE chat_id WITH YOUR CHAT ID

#if ESP32
const char* ca = \
                 "-----BEGIN CERTIFICATE-----\n" \
                 "MIIGvjCCBaagAwIBAgIIDQMjoJC2gEIwDQYJKoZIhvcNAQELBQAwgbQxCzAJBgNV\n" \
                 "BAYTAlVTMRAwDgYDVQQIEwdBcml6b25hMRMwEQYDVQQHEwpTY290dHNkYWxlMRow\n" \
                 "GAYDVQQKExFHb0RhZGR5LmNvbSwgSW5jLjEtMCsGA1UECxMkaHR0cDovL2NlcnRz\n" \
                 "LmdvZGFkZHkuY29tL3JlcG9zaXRvcnkvMTMwMQYDVQQDEypHbyBEYWRkeSBTZWN1\n" \
                 "cmUgQ2VydGlmaWNhdGUgQXV0aG9yaXR5IC0gRzIwHhcNMjAwMzI0MTM0ODE3WhcN\n" \
                 "MjIwNTIzMTYxNzM4WjA+MSEwHwYDVQQLExhEb21haW4gQ29udHJvbCBWYWxpZGF0\n" \
                 "ZWQxGTAXBgNVBAMTEGFwaS50ZWxlZ3JhbS5vcmcwggEiMA0GCSqGSIb3DQEBAQUA\n" \
                 "A4IBDwAwggEKAoIBAQC0oxaeXFfJiWXt6ngLropYL65ayG5JjfxXpZiIeC4LPEA8\n" \
                 "IS6alJgzp+NCp4X60HOEARxyOTcjtVYdQ6VxFAgkpTnM3lhTlI4qQqdOLQcynrqL\n" \
                 "0yqpnsDjzpoQlkVYesceRRQjkrtUgoiUSba+gSEAKW3Jzos5Otw1FdnrR5zvugkO\n" \
                 "FuTZ63Iw+kmrmDF8s6wrKZGHCEFyXjXHhwQi9Uh2MG2I3/KlKRNws4cC1WtYsehz\n" \
                 "x+TveYakB19ntHmNpCUBgozgMBfLS1z760wSUbnJBB9+0vi69TWNihw3gvAVcwBu\n" \
                 "PRx2iwF0gT3kLKfML2bcRKgnP+rQp6jxy+raBzi9AgMBAAGjggNHMIIDQzAMBgNV\n" \
                 "HRMBAf8EAjAAMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjAOBgNVHQ8B\n" \
                 "Af8EBAMCBaAwOAYDVR0fBDEwLzAtoCugKYYnaHR0cDovL2NybC5nb2RhZGR5LmNv\n" \
                 "bS9nZGlnMnMxLTE4MjMuY3JsMF0GA1UdIARWMFQwSAYLYIZIAYb9bQEHFwEwOTA3\n" \
                 "BggrBgEFBQcCARYraHR0cDovL2NlcnRpZmljYXRlcy5nb2RhZGR5LmNvbS9yZXBv\n" \
                 "c2l0b3J5LzAIBgZngQwBAgEwdgYIKwYBBQUHAQEEajBoMCQGCCsGAQUFBzABhhho\n" \
                 "dHRwOi8vb2NzcC5nb2RhZGR5LmNvbS8wQAYIKwYBBQUHMAKGNGh0dHA6Ly9jZXJ0\n" \
                 "aWZpY2F0ZXMuZ29kYWRkeS5jb20vcmVwb3NpdG9yeS9nZGlnMi5jcnQwHwYDVR0j\n" \
                 "BBgwFoAUQMK9J47MNIMwojPX+2yz8LQsgM4wMQYDVR0RBCowKIIQYXBpLnRlbGVn\n" \
                 "cmFtLm9yZ4IUd3d3LmFwaS50ZWxlZ3JhbS5vcmcwHQYDVR0OBBYEFAUU0MDD7w6U\n" \
                 "uyGZ56M2XCh3MJbVMIIBfgYKKwYBBAHWeQIEAgSCAW4EggFqAWgAdQCkuQmQtBhY\n" \
                 "FIe7E6LMZ3AKPDWYBPkb37jjd80OyA3cEAAAAXEMzRPeAAAEAwBGMEQCIA7+nU6c\n" \
                 "b04gkC/blToDBZ2yqQdYl7e8RI4qGS8yVcCgAiA4wlR38PqzXS/91cJE5tUjc3hK\n" \
                 "1e1Rq6fN78SGBUI9egB3AO5Lvbd1zmC64UJpH6vhnmajD35fsHLYgwDEe4l6qP3L\n" \
                 "AAABcQzNF3AAAAQDAEgwRgIhAOYFG3gzQVO6DdIICHs8nayUligbbKC/favMIMEK\n" \
                 "tVTSAiEAwhzLa1pcYMpIGg1hclL4yA6J7GSn5GjehT2UxrJh1IoAdgBWFAaaL9fC\n" \
                 "7NP14b1Esj7HRna5vJkRXMDvlJhV1onQ3QAAAXEMzRoEAAAEAwBHMEUCIQCEk/Ea\n" \
                 "UfSzBIUw4qKGl0Qsf2ekOD6yhzVkUqvDOgxH4wIgIMiZyFyoiowB+n3oi6XOr7vk\n" \
                 "S3rEpdXgxEp/sqj3egMwDQYJKoZIhvcNAQELBQADggEBACgT72RgemRjXNh8HcSO\n" \
                 "cBb9UgawTXYHtOUKFuzDA8uylHtI5jrnGARu6UVqi1NQ2jeHbXRJtFLaS4Upu5tT\n" \
                 "Vj8O7pSls046PzNdf4TztultzGhkICJFGqrbDfQ/vmG/Sp+2PMPjoM/pjncD/qDW\n" \
                 "OgQjHW0I/pNYk8SpfwHdVHI0JVJ3iVrr2/PHiX7sjoXAY1DM1Mov+29RmiPECDz+\n" \
                 "VAahEUoSa3YvkDlRAHpC6gHLaBlqMirVb3AyK51aueeOR3OyxJqlLYkEmR9G81nu\n" \
                 "mZYZ19HF8bXdIVvQs0+P8gPIql7hz9Ygmw90rcWvQOC4K4Z//oxezoM0SNsdiufy\n" \
                 "OiI=\n" \
                 "-----END CERTIFICATE-----\n";
#endif
