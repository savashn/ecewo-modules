#ifndef ECEWO_SESSION_H
#define ECEWO_SESSION_H

#include <time.h>
#include <stdint.h>
#include "ecewo.h"
#include "ecewo-cookie.h"

#define SESSION_ID_LEN 32       // Length of the session ID (32 characters)
#define MAX_SESSIONS_DEFAULT 10 // Default initial capacity for sessions

// Session structure to hold session information
typedef struct
{
    char id[SESSION_ID_LEN + 1]; // Unique session ID (32 bytes + null terminator)
    char *data;                  // URL-encoded key-value pairs
    time_t expires;              // Expiration time of the session (UNIX timestamp)
} Session;

// Initialize the session system with default capacity
// Returns: 1 on success, 0 on failure
int session_init(void);

// Clean up and free all session resources
void session_cleanup(void);

// Create a new session with specified max age in seconds
// Returns: Pointer to new session on success, NULL on failure
Session *session_create(int max_age);

// Find a session by its ID
// Returns: Pointer to session if found and not expired, NULL otherwise
Session *session_find(const char *id);

// Set a key-value pair in the session's data (URL encoded storage)
// Parameters:
//   sess: Session to modify
//   key: Key name (will be URL encoded)
//   value: Value (will be URL encoded)
void session_value_set(Session *sess, const char *key, const char *value);

// Get a value from session by key (returns decoded value)
// Returns: Decoded value string or NULL if not found
// Note: Returned string is malloc'd and should be freed by caller
char *session_value_get(Session *sess, const char *key);

// Remove a key-value pair from session
void session_value_remove(Session *sess, const char *key);

// Free a session and its associated resources
// Clears session ID, expiration time, and frees session data
void session_free(Session *sess);

// Get the authenticated session from request cookies
// Returns: Session if found and authenticated, NULL otherwise
Session *session_get(Req *req);

// Send session cookie to the client with default secure options
void session_send(Res *res, Session *sess, Cookie *options);

// Delete the session both from the client and the memory
void session_destroy(Res *res, Session *sess, Cookie *options);

// Print all registered sessions to stdout (debug purposes)
void session_print_all(void);

#endif
