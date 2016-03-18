#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ssl_parse_record.h"

#define DROPDOWN(i, length, state) (state)++; if (++(i)>=(length)) break

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void
parse_server_hello(
    const struct Banner1* banner1,
    void* banner1_private,
    struct ProtocolState* pstate,
    const unsigned char* px, size_t length,
    struct BannerOutput* banout,
    struct InteractiveData* more) {

    struct SSL_SERVER_HELLO* hello = &pstate->sub.ssl.x.server_hello;
    unsigned state = hello->state;
    unsigned remaining = hello->remaining;
    unsigned i;
    enum {
        VERSION_MAJOR, VERSION_MINOR,
        TIME0, TIME1, TIME2, TIME3,
        RANDOM,
        SESSION_LENGTH, SESSION_ID,
        CIPHER0, CIPHER1,
        COMPRESSION,
        LENGTH0, LENGTH1,
        EXT_TAG0, EXT_TAG1,
        EXT_LEN0, EXT_LEN1,
        EXT_DATA,
        EXT_DATA_HEARTBEAT,
        UNKNOWN,
    };

    UNUSEDPARM(banout);
    UNUSEDPARM(banner1_private);
    UNUSEDPARM(banner1);
    UNUSEDPARM(more);

    /* What this structure looks like in ASN.1 format
       struct {
           ProtocolVersion server_version;
           Random random;
           SessionID session_id;
           CipherSuite cipher_suite;
           CompressionMethod compression_method;
       } ServerHello;
    */

    /* 'for all bytes in the packet...' */
    for (i = 0; i < length; i++)
        switch (state) {
            case VERSION_MAJOR:
                hello->version_major = px[i];
                DROPDOWN(i, length, state);

            case VERSION_MINOR:
                hello->version_minor = px[i];
                // BANNER_VERSION(banout, hello->version_major, hello->version_minor);
                if (banner1->is_poodle_sslv3) {
                    // banout_append(banout, PROTO_VULN, " POODLE ", AUTO_LEN);
                }
                if (hello->version_major > 3 || hello->version_minor > 4) {
                    state = UNKNOWN;
                    break;
                }
                hello->timestamp = 0;
                DROPDOWN(i, length, state);

            case TIME0:
                hello->timestamp <<= 8;
                hello->timestamp |= px[i];
                DROPDOWN(i, length, state);
            case TIME1:
                hello->timestamp <<= 8;
                hello->timestamp |= px[i];
                DROPDOWN(i, length, state);
            case TIME2:
                hello->timestamp <<= 8;
                hello->timestamp |= px[i];
                DROPDOWN(i, length, state);
            case TIME3:
                hello->timestamp <<= 8;
                hello->timestamp |= px[i];
                remaining = 28;
                DROPDOWN(i, length, state);
            case RANDOM: {
                /* do our typical "skip" logic to skip this
                 * 32 byte field */
                unsigned len = (unsigned) length - i;
                if (len > remaining)
                    len = remaining;

                remaining -= len;
                i += len - 1;

                if (remaining != 0) {
                    break;
                }
            }
                DROPDOWN(i, length, state);

            case SESSION_LENGTH:
                remaining = px[i];
                DROPDOWN(i, length, state);

            case SESSION_ID: {
                unsigned len = (unsigned) length - i;
                if (len > remaining)
                    len = remaining;

                remaining -= len;
                i += len - 1;

                if (remaining != 0) {
                    break;
                }
            }
                hello->cipher_suite = 0;
                DROPDOWN(i, length, state);

            case CIPHER0:
                hello->cipher_suite <<= 8;
                hello->cipher_suite |= px[i];
                DROPDOWN(i, length, state);

            case CIPHER1:
                hello->cipher_suite <<= 8;
                hello->cipher_suite |= px[i]; /* cipher-suite recorded here */
                // BANNER_CIPHER(banout, hello->cipher_suite);
                DROPDOWN(i, length, state);

            case COMPRESSION:
                hello->compression_method = px[i];
                DROPDOWN(i, length, state);

            case LENGTH0:
                remaining = px[i];
                DROPDOWN(i, length, state);

            case LENGTH1:
                remaining <<= 8;
                remaining |= px[i];
                DROPDOWN(i, length, state);

            case EXT_TAG0:
            ext_tag:
                if (remaining < 4) {
                    state = UNKNOWN;
                    continue;
                }
                hello->ext_tag = px[i] << 8;
                remaining--;
                DROPDOWN(i, length, state);

            case EXT_TAG1:
                hello->ext_tag |= px[i];
                remaining--;
                DROPDOWN(i, length, state);

            case EXT_LEN0:
                hello->ext_remaining = px[i] << 8;
                remaining--;
                DROPDOWN(i, length, state);
            case EXT_LEN1:
                hello->ext_remaining |= px[i];
                remaining--;
                switch (hello->ext_tag) {
                    case 0x000f: /* heartbeat */
                        state = EXT_DATA_HEARTBEAT;
                        continue;
                }
                DROPDOWN(i, length, state);

            case EXT_DATA:
                if (hello->ext_remaining == 0) {
                    state = EXT_TAG0;
                    goto ext_tag;
                }
                if (remaining == 0) {
                    state = UNKNOWN;
                    continue;
                }
                remaining--;
                hello->ext_remaining--;
                continue;

            case EXT_DATA_HEARTBEAT:
                if (hello->ext_remaining == 0) {
                    state = EXT_TAG0;
                    goto ext_tag;
                }
                if (remaining == 0) {
                    state = UNKNOWN;
                    continue;
                }
                remaining--;
                hello->ext_remaining--;
                if (px[i]) {
                    // banout_append(banout, PROTO_VULN, "SSL[heartbeat] ", 15);
                }
                state = EXT_DATA;
                continue;


            case UNKNOWN:
            default:
                i = (unsigned) length;
        }

    hello->state = state;
    hello->remaining = remaining;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void
parse_server_cert(
    const struct Banner1* banner1,
    void* banner1_private,
    struct ProtocolState* pstate,
    const unsigned char* px, size_t length,
    struct BannerOutput* banout) {

    struct SSL_SERVER_CERT* data = &pstate->sub.ssl.x.server_cert;
    unsigned state = data->state;
    unsigned remaining = data->remaining;
    unsigned cert_remaining = data->sub.remaining;
    unsigned i;
    enum {
        LEN0, LEN1, LEN2,
        CLEN0, CLEN1, CLEN2,
        CERT,
        UNKNOWN,
    };

    UNUSEDPARM(banner1);
    UNUSEDPARM(banner1_private);

    for (i = 0; i < length; i++)
        switch (state) {
            case LEN0:
                remaining = px[i];
                DROPDOWN(i, length, state);
            case LEN1:
                remaining = remaining * 256 + px[i];
                DROPDOWN(i, length, state);
            case LEN2:
                remaining = remaining * 256 + px[i];
                DROPDOWN(i, length, state);

            case CLEN0:
                if (remaining < 3) {
                    state = UNKNOWN;
                    continue;
                }
                cert_remaining = px[i];
                remaining--;
                DROPDOWN(i, length, state);
            case CLEN1:
                cert_remaining = cert_remaining * 256 + px[i];
                remaining--;
                DROPDOWN(i, length, state);
            case CLEN2:
                cert_remaining = cert_remaining * 256 + px[i];
                remaining--;
                if (banner1->is_capture_cert) {
                    // banout_init_base64(&pstate->base64);
                }

                {
                    // unsigned count = data->x509.count;
                    // memset(&data->x509, 0, sizeof(data->x509));
                    // x509_decode_init(&data->x509, cert_remaining);
                    // data->x509.count = (unsigned char) count + 1;
                }
                DROPDOWN(i, length, state);

            case CERT: {
                unsigned len = (unsigned) length - i;
                if (len > remaining)
                    len = remaining;
                if (len > cert_remaining)
                    len = cert_remaining;

                /* parse the certificate */
                if (banner1->is_capture_cert) {
                    // banout_append_base64(banout,
                    //                      PROTO_X509_CERT,
                    //                      px + i, len,
                    //                      &pstate->base64);
                }

                // x509_decode(&data->x509, px + i, len, banout);


                remaining -= len;
                cert_remaining -= len;
                i += len - 1;

                if (cert_remaining == 0) {
                    /* We've reached the end of the certificate, so make
                     * a record of it */
                    if (banner1->is_capture_cert) {
                        // banout_finalize_base64(banout,
                        //                        PROTO_X509_CERT,
                        //                        &pstate->base64);
                        // banout_end(banout, PROTO_X509_CERT);
                    }
                    state = CLEN0;
                    if (remaining == 0) {
                        if (!banner1->is_heartbleed)
                            pstate->is_done = 1;
                    }
                }
            }
                break;


            case UNKNOWN:
            default:
                i = (unsigned) length;
        }

    data->state = state;
    data->remaining = remaining;
    data->sub.remaining = cert_remaining;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void
parse_handshake(
    const struct Banner1* banner1,
    void* banner1_private,
    struct ProtocolState* pstate,
    const unsigned char* px, size_t length,
    struct BannerOutput* banout,
    struct InteractiveData* more) {

    struct SSLRECORD* ssl = &pstate->sub.ssl;
    unsigned state = ssl->handshake.state;
    unsigned remaining = ssl->handshake.remaining;
    unsigned i;
    enum {
        START,
        LENGTH0, LENGTH1, LENGTH2,
        CONTENTS,
        UNKNOWN,
    };

    /*
     * `for all bytes in the segment`
     *   `do a state transition for that byte`
     */
    for (i = 0; i < length; i++)
        switch (state) {

            /* There are 20 or so handshaking sub-messages, indicates by it's own
             * 'type' field, which we parse out here */
            case START:
                if (px[i] & 0x80) {
                    state = UNKNOWN;
                    break;
                }
                /* remember the 'type' field for use later in the CONTENT state */
                ssl->handshake.type = px[i];

                /* initialize the state variable that will be used by the inner
                 * parsers */
                ssl->x.all.state = 0;
                DROPDOWN(i, length, state);

                /* This grabs the 'length' field. Note that unlike other length fields,
                 * this one is 3 bytes long. That's because a single certificate
                 * packet can contain so many certificates in a chain that it exceeds
                 * 64k kilobytes in size. */
            case LENGTH0:
                remaining = px[i];
                DROPDOWN(i, length, state);
            case LENGTH1:
                remaining <<= 8;
                remaining |= px[i];
                DROPDOWN(i, length, state);
            case LENGTH2:
                remaining <<= 8;
                remaining |= px[i];

                /* If we get a "server done" response, then it's a good time to
                 * send the heartbleed request. Note that these are usually zero
                 * length, so we can't process this below in the CONTENT state
                 * but have to do it here at the end of the LENGTH2 state */
                if (ssl->handshake.type == 2 && banner1->is_heartbleed) {
                    static const char heartbleed_request[] =
                        "\x15\x03\x02\x00\x02\x01\x80"
                            "\x18\x03\x02\x00\x03\x01" "\x40\x00";
                    more->payload = heartbleed_request;
                    more->length = sizeof(heartbleed_request) - 1;
                }
                DROPDOWN(i, length, state);

                /* This parses the contents of the handshake. This parser just skips
                 * the data, in the same way as explained in the "ssl_parse_record()"
                 * function at its CONTENT state. We may pass the fragment to an inner
                 * parser, but whatever the inner parser does is independent from this
                 * parser, and has no effect on this parser
                 */
            case CONTENTS: {
                unsigned len = (unsigned) length - i;
                if (len > remaining)
                    len = remaining;

                switch (ssl->handshake.type) {
                    case 0: /* hello request*/
                    case 1: /* client hello */
                    case 3: /* DTLS hello verify request */
                    case 4: /* new session ticket */
                    case 12: /* server key exchange */
                    case 13: /* certificate request */
                    case 14: /* server done */
                    case 15: /* certificate verify */
                    case 16: /* client key exchange */
                    case 20: /* finished */
                    case 22: /* certificate status */
                    default:
                        /* don't parse these types, just skip them */
                        break;

                    case 2: /* server hello */
                        parse_server_hello(banner1,
                                           banner1_private,
                                           pstate,
                                           px + i, len,
                                           banout,
                                           more);
                        break;
                    case 11: /* server certificate */
                        parse_server_cert(banner1,
                                          banner1_private,
                                          pstate,
                                          px + i, len,
                                          banout);
                        break;
                }

                remaining -= len;
                i += len - 1;

                if (remaining == 0)
                    state = START;
            }

                break;
            case UNKNOWN:
            default:
                i = (unsigned) length;
        }

    ssl->handshake.state = state;
    ssl->handshake.remaining = remaining;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void
parse_heartbeat(
    const struct Banner1* banner1,
    void* banner1_private,
    struct ProtocolState* pstate,
    const unsigned char* px, size_t length,
    struct BannerOutput* banout,
    struct InteractiveData* more) {

    struct SSLRECORD* ssl = &pstate->sub.ssl;
    unsigned state = ssl->handshake.state;
    unsigned remaining = ssl->handshake.remaining;
    unsigned i;
    enum {
        START,
        LENGTH0, LENGTH1,
        CONTENTS,
        UNKNOWN,
    };

    UNUSEDPARM(more);
    UNUSEDPARM(banner1_private);

    /*
     * `for all bytes in the segment`
     *   `do a state transition for that byte`
     */
    for (i = 0; i < length; i++)
        switch (state) {

            /* this is the 'type' field for the hearbeat. There are only two
             * values, '1' for request and '2' for response. Anything else indicates
             * that either the data was corrupted, or else it is encrypted.
             */
            case START:
                if (px[i] < 1 || 2 < px[i]) {
                    state = UNKNOWN;
                    break;
                }
                ssl->handshake.type = px[i];
                DROPDOWN(i, length, state);

                /* Grab the two byte length field */
            case LENGTH0:
                remaining = px[i];
                DROPDOWN(i, length, state);
            case LENGTH1:
                remaining <<= 8;
                remaining |= px[i];

                /* `if heartbeat response ` */
                if (ssl->handshake.type == 2) {

                    /* if we have a non-trivial amount of data in the response, then
                     * it means the "bleed" attempt succeeded. */
                    if (remaining >= 16)
                        // banout_append(banout, PROTO_VULN, "SSL[HEARTBLEED] ", 16);

                        /* if we've been configured to "capture" the heartbleed contents,
                         * then initialize the BASE64 encoder */
                    if (banner1->is_capture_heartbleed) {
                        // banout_init_base64(&pstate->base64);
                        // banout_append(banout, PROTO_HEARTBLEED, "", 0);
                    }
                }
                DROPDOWN(i, length, state);

                /* Here is where we parse the contents of the heartbeat. This is the same
                 * skipping logic as the CONTENTS state within the ssl_parse_record()
                 * function.*/
            case CONTENTS: {
                unsigned len = (unsigned) length - i;
                if (len > remaining)
                    len = remaining;

                /* If this is a RESPONSE, and we've been configured to CAPTURE
                 * hearbleed responses, then we write the bleeding bytes in
                 * BASE64 into the banner system. The user will be able to
                 * then do research on those bleeding bytes */
                if (ssl->handshake.type == 2 && banner1->is_capture_heartbleed) {
                    // banout_append_base64(banout,
                    //                      PROTO_HEARTBLEED,
                    //                      px + i, len,
                    //                      &pstate->base64);
                }

                remaining -= len;
                i += len - 1;

                if (remaining == 0)
                    state = UNKNOWN; /* padding */
            }

                break;

                /* We reach this state either because the hearbeat data is corrupted or
                 * encrypted, or because we've reached the padding area after the
                 * heartbeat */
            case UNKNOWN:
            default:
                i = (unsigned) length;
        }

    /* not the handshake protocol, but we re-use their variables */
    ssl->handshake.state = state;
    ssl->handshake.remaining = remaining;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void
parse_alert(
    const struct Banner1* banner1,
    void* banner1_private,
    struct ProtocolState* pstate,
    const unsigned char* px, size_t length,
    struct BannerOutput* banout,
    struct InteractiveData* more) {

    struct SSLRECORD* ssl = &pstate->sub.ssl;
    unsigned state = ssl->handshake.state;
    unsigned remaining = ssl->handshake.remaining;
    unsigned i;
    enum {
        START,
        DESCRIPTION,
        UNKNOWN,
    };

    UNUSEDPARM(more);
    UNUSEDPARM(banner1_private);

    /*
     * `for all bytes in the segment`
     *   `do a state transition for that byte `
     */
    for (i = 0; i < length; i++)
        switch (state) {
            case START:
                ssl->x.server_alert.level = px[i];
                DROPDOWN(i, length, state);

            case DESCRIPTION:
                ssl->x.server_alert.description = px[i];
                if (banner1->is_poodle_sslv3 && ssl->x.server_alert.level == 2) {
                    char foo[64];

                    /* fatal error */
                    switch (ssl->x.server_alert.description) {
                        case 86:
                            // if (!banout_is_contains(banout, PROTO_SAFE, "TLS_FALLBACK_SCSV"))
                            //     banout_append(banout, PROTO_SAFE,
                            //                   "poodle[TLS_FALLBACK_SCSV] ", AUTO_LEN);
                            break;
                        case 40:
                            // if (!banout_is_contains(banout, PROTO_SAFE, "TLS_FALLBACK_SCSV"))
                            //     banout_append(banout, PROTO_SAFE,
                            //                   "poodle[no-SSLv3] ", AUTO_LEN);
                            break;
                        default:
                            // banout_append(banout, PROTO_SAFE,
                            //               "poodle[no-SSLv3] ", AUTO_LEN);
                            sprintf_s(foo, sizeof(foo), " ALERT(0x%02x%02x) ",
                                      ssl->x.server_alert.level,
                                      ssl->x.server_alert.description
                            );

                            // banout_append(banout, PROTO_SSL3, foo, AUTO_LEN);
                            break;
                    }
                } else {
                    char foo[64];
                    sprintf_s(foo, sizeof(foo), " ALERT(0x%02x%02x) ",
                              ssl->x.server_alert.level,
                              ssl->x.server_alert.description
                    );

                    // banout_append(banout, PROTO_SSL3, foo, AUTO_LEN);
                }
                DROPDOWN(i, length, state);

            case UNKNOWN:
            default:
                i = (unsigned) length;
        }

    /* not the handshake protocol, but we re-use their variables */
    ssl->handshake.state = state;
    ssl->handshake.remaining = remaining;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void
ssl_parse_record(
    const struct Banner1* banner1,
    void* banner1_private,
    struct ProtocolState* pstate,
    const unsigned char* px, size_t length,
    struct BannerOutput* banout,
    struct InteractiveData* more) {

    unsigned state = pstate->state;
    unsigned remaining = pstate->remaining;
    struct SSLRECORD* ssl = &pstate->sub.ssl;
    unsigned i;
    enum {
        START,
        VERSION_MAJOR,
        VERSION_MINOR,
        LENGTH0, LENGTH1,
        CONTENTS,
        UNKNOWN,
    };

    /*
     * `for all bytes in the segment`
     *   `do a state transition for that byte`
     */
    for (i = 0; i < length; i++)
        switch (state) {

            /*
             * The initial state parses the "type" byte. There are only a few types
             * defined so far, the values 20-25, but more can be defined in the
             * future. The standard explicity says that they must be lower than 128,
             * so if the high-order bit is set, we know that the byte is invalid,
             * and that something is wrong.
             */
            case START:
                if (px[i] & 0x80) {
                    state = UNKNOWN;
                    break;
                }
                if (ssl->type != px[i]) {
                    ssl->type = px[i];

                    /* this is for some minimal fragmentation/reassembly */
                    ssl->handshake.state = 0;
                }
                DROPDOWN(i, length, state);

                /* This is the major version number, which must be the value '3',
                 * which means both SSLv3 and TLSv1. This parser doesn't support
                 * earlier versions of SSL. */
            case VERSION_MAJOR:
                if (px[i] != 3) {
                    state = UNKNOWN;
                    break;
                }
                ssl->version_major = px[i];
                DROPDOWN(i, length, state);

                /* This is the minor version number. It's a little weird:
                 * 0 = SSLv3.0
                 * 1 = TLSv1.0
                 * 2 = TLSv1.1
                 * 3 = TLSv1.2
                 * 4 = TLSv1.3
                 */
            case VERSION_MINOR:
                ssl->version_minor = px[i];
                DROPDOWN(i, length, state);

                /* This is the length field. In theory, it can be the full 64k bytes
                 * in length, but typical implements limit it to 16k */
            case LENGTH0:
                remaining = px[i] << 8;
                DROPDOWN(i, length, state);
            case LENGTH1:
                remaining |= px[i];
                DROPDOWN(i, length, state);
                ssl->handshake.state = 0;

                /*
                 * This state parses the "contents" of a record. What we do here is at
                 * this level of the parser is that we calculate a sub-segment size,
                 * which is bounded by either the number of bytes in this records (when
                 * there are multiple records per packet), or the packet size (when the
                 * record exceeds the size of the packet).
                 * We then pass this sug-segment to the inner content parser. However, the
                 * inner parser has no effect on what happens in this parser. It's wholy
                 * indpedent, doing it's own thing.
                 */
            case CONTENTS: {
                unsigned len;

                /* Size of this segment is either the bytes remaining in the
                 * current packet, or the bytes remaining in the record */
                len = (unsigned) length - i;
                if (len > remaining)
                    len = remaining;

                /* Do an inner-parse of this segment. Note that the inner-parser
                 * has no effect on this outer record parser */
                switch (ssl->type) {
                    case 20: /* change cipher spec */
                        break;
                    case 21: /* alert */
                        /* encrypted, usually, but if we get one here, it won't
                         * be encrypted */
                        parse_alert(banner1,
                                    banner1_private,
                                    pstate,
                                    px + i, len,
                                    banout,
                                    more);
                        break;
                    case 22: /* handshake */
                        parse_handshake(banner1,
                                        banner1_private,
                                        pstate,
                                        px + i, len,
                                        banout,
                                        more);
                        break;
                    case 23: /* application data */
                        /* encrypted, always*/
                        break;
                    case 24: /* heartbeat */
                        /* enrypted, in theory, but not practice */
                        parse_heartbeat(banner1,
                                        banner1_private,
                                        pstate,
                                        px + i, len,
                                        banout,
                                        more);
                        break;
                }

                /* Skip ahead the number bytes in this segment. This makes the
                 * parser very fast, because we aren't actually doing a single
                 * byte at a time, but skipping forward large number of bytes
                 * at a time -- except for the 5 byte headers */
                remaining -= len;
                i += len - 1; /* if 'len' is zero, this still works */

                /* Once we've exhausted the contents of record, go back to the
                 * start parsing the next record */
                if (remaining == 0)
                    state = START;
            }
                break;

                /* We reach the state when the protocol has become corrupted, such as in
                 * those cases where it's not SSL */
            case UNKNOWN:
            default:
                i = (unsigned) length;
        }

    pstate->state = state;
    pstate->remaining = remaining;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    struct Banner1* banner1 = malloc(sizeof(struct Banner1));
    void* banner1_private = malloc(sizeof(void));
    struct ProtocolState* pstate = malloc(sizeof(struct ProtocolState));
    unsigned char* px = malloc(sizeof(char) * 42);
    size_t length = 42;
    struct BannerOutput* banout = malloc(sizeof(struct BannerOutput));
    struct InteractiveData* more = malloc(sizeof(struct InteractiveData));

    ssl_parse_record(banner1, banner1_private, pstate, px, length, banout, more);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
