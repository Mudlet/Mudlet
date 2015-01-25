#pragma once

#include <mutex>
#include <jni.h>

class Profile;
class QString;

/**
 * @brief A handle and operations to a DirectByteBuffer created from Java.
 *
 * This class represents a DirectBuffer created by the Java side.  On initialization, the
 * Java side will create a direct byte buffer for sharing memory with the C++ side.  The
 * C++ side will grab the address and capacity and store them in this class.
 *
 * Concepts
 * ========
 *
 * The general contract is that C++ will call into Java on a single thread per session via
 * the session's void event() method.  This means event() is not re-entrant.  The method
 * is parameterless and returns void.  C++ and Java communicate via a shared memory buffer
 * instead.  There is always one buffer per session.
 *
 * After a completed event() call, the first part of the buffer will contain only C++
 * information followed by only Java information until a CMD_END is encountered.  The
 * buffer is never zero'd so the remaining information is possibly garbage from a previous
 * invocation.
 *
 * C++ and Java both maintain markers, or integer offsets as they proceed through the
 * buffer which are not kept in sync.
 *
 * C++ only writes one single event and Java writes as many commands as it likes and then
 * must write a @c CMD_END to terminate.  Events and commands are typed and these types are
 * represented by 32 bit unsigned integer.
 *
 * Each event or command has a fixed number, order, and type of argument that each side
 * knows in order to pull off the buffer correctly.
 *
 * Each write advanced the @c marker by the relevant amount, as does each read.
 *
 * For @c EVENT_INIT, C++ does not process Java's commands.
 *
 * Event Procedure
 * ===============
 *
 * - C++ sets its @c marker to 0.
 * - C++ writes an integer representing the event
 * - C++ writes the arguments for the event
 * - C++'s @c marker is now one byte after the last byte of the last argument
 * - C++ calls, via JNI, the relevant Session.event() in Java
 * - Thread control is handed to Java
 * - Java sets its @c marker to 0
 * - Java reads the first integer, the event type.
 * - Java reads in all the correct arguments
 * - Java's marker is now equivalent to C++'s marker
 * - Java writes as many commands as it likes
 * - To write a command, Java first writes an int representing the command type
 * - Java then writes the corresponding representations of the typed arguments
 * - Java =must= write a final CMD_END
 * - Java's marker is now at the byte after the final CMD_END byte.
 * - Control then returns to C++
 * - C++ loops through the commands by reading the command int
 * - Then it reads the relevant arguments, if any
 * - C++ then processes that command in order.
 * - C++ stops processing when it encounters CMD_END
 * - C++'s marker now equals Java's marker again
 *
 * Encodings
 * =========
 *
 * # int
 *
 * Integers are stored big-endian across 4 bytes.  They are always signed.
 *
 * # string
 *
 * The first four bytes are the length of the string stored as an integer.  Strings are
 * assumed to be low-order ASCII with one byte per character.  The final byte of the string
 * is always a '\0' character in order to create C++ friendly null terminated strings.
 * The length of the string includes this byte.
 *
 * # char
 *
 * A single byte is used to represent a char.  It is assumed to be 7 bit ASCII.
 *
 * TODO: make class safe for buffer overflows
 * TODO: make platform independent, currently C++ side assumes little endian and the Java
 * side assumes big endian.
 * TODO: make character encoding safe, currently assumes strings are always 7 bit ASCII
 * TODO: Java side assumes UTF-8, uh roh!
 *
 */
class DirectBuffer {
public:
    DirectBuffer(void* address,int capacity) : address(address), capacity(capacity) {
        bytes = (char*) address;
        marker = 0;
    }

    /**
     * @brief read the next integer from the buffer
     *
     * @return the next 32 bit signed integer
     */
    int nextInt() {
        // TODO make this platform independent
        int i = 0;
        i += bytes[marker++] << 24;
        i += bytes[marker++] << 16;
        i += bytes[marker++] << 8;
        i += bytes[marker++];
        return i;
    }

    char nextChar() {
        return bytes[marker++];
    }

    char * nextString() {
        // TODO make this platform independent
        int length = nextInt();
        char * p = bytes + marker;
        marker += length;
        return p;
    }

    void write(const QString &);
    void write(const char *ch);

    /**
     * @brief push an integer onto the buffer
     *
     */
    void write(int);
    void clear();

private:
    char * bytes;
    void * address;
    int marker;
    int capacity;
};

/**
 * @brief This class represents maintains a one-to-one correspondence between
 * a Host and a Java-side session.
 *
 * The life of this class should be the same as the host.  It does not have
 * to be the same as the embedded JVM nor any classloaders.
 */
class Java
{
public:
    Java(Profile * const,const QString &);
    ~Java();

    void                handleLine(const QString &, int );

private:
    void                createVM();
    JNIEnv *            getJNIEnv();
    void                handleCommand(int cmd);
    void                fatalException();

    void                getSession();
    void                getBuffer();

    void                eventInit();

    void                cmdEcho();
    void                cmdSend();

    JavaVMInitArgs      vm_args;
    JavaVMOption*       options;

    jclass              gateway;
    jclass              sessionClass;

    jmethodID           eventMethod;
    jobject             session;

    Profile *              host;
    DirectBuffer *      buffer;
    QString             hostname;

    std::mutex          lock;
};
