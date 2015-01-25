#include <QtDebug>
#include <QtGlobal>
#include <QDir>
#include <thread>
#include <type_traits>
#include <string>


#include "Java.h"
#include "Host.h"
#include "TConsole.h"

static JavaVM *jvm = NULL;
static JNIEnv *env = NULL;
static std::mutex jvmLock;

/**
 * @brief java_count is a ref count on the number of java sessions using the JVM
 * There can only be one JVM embedded into the process.  All hosts and sessions must
 * share this.
 */
static int java_count = 0;

static const int CMD_END  = 0x000;
static const int CMD_ECHO = 0x001;
static const int CMD_SEND = 0x002;

static const int EVENT_INIT = 0x000;
static const int EVENT_LINE = 0x001;

void DirectBuffer::write(const char *ch) {

    qDebug() << "writing string: " << ch;

    qDebug() << "narker: " << marker;
    int len = strlen(ch) + 1;
    qDebug() << "strln: " << len;
    write(len);
    qDebug() << "narker: " << marker;

    strcpy(bytes + marker,ch);
    marker += len;
    qDebug() << "narker: " << marker;
}

void DirectBuffer::write(const QString &qs) {
    write(qs.toStdString().c_str());
}

void DirectBuffer::write(int i) {
    bytes[marker++] = (i >> 24) && 0xF;
    bytes[marker++] = (i >> 16) && 0xF;
    bytes[marker++] = (i >> 8) && 0xF;
    bytes[marker++] = i && 0xF;
}

void DirectBuffer::clear() {
    marker = 0;
}


Java::Java(Host * const host, const QString &hostname)
: host(host)
, hostname(hostname)
{
    if(jvmLock.try_lock() == false) {
        qDebug() << "jvm locked!";
        exit(-1);
        jvmLock.lock();
    }

    if(java_count++ == 0) {
        createVM();
    }


    env = getJNIEnv();

    gateway = env->FindClass("mudlet/jni/Gateway");
    sessionClass = env->FindClass("mudlet/session/Session");

    if (gateway == NULL || session == NULL) {
        qDebug() << "failed to find classes";
        exit(-1);
        return;
    }

    getSession();
    getBuffer();
    eventInit();
    jvmLock.unlock();
}

Java::~Java() {
    jvmLock.lock();

    env = getJNIEnv();
    qDebug() << "deleting session reference";
    env->DeleteGlobalRef(session);

    delete buffer;


    if (--java_count == 0) {
        qDebug() << "destroying the JVM";
        jvm->DestroyJavaVM();
    }

    jvmLock.unlock();
}


JNIEnv * Java::getJNIEnv(){
    jvm->GetEnv((void **) &env,NULL);

    if(env == NULL) {
        auto id = std::this_thread::get_id();
        qDebug() << "Attaching " << &id;
        jvm->AttachCurrentThread((void **) &env,NULL);
    }

    Q_ASSERT(env != NULL);

    if(env == NULL) {
        qCritical() << "env is NULL!";
        exit(-1);
    }

    return env;
}

void Java::getSession() {
    auto method = env->GetStaticMethodID(gateway, "newSession", "()Lmudlet/session/Session;");

    if(method == NULL) {
        qDebug() << "failed to find newSession";
        exit(-1);
        return;
    }

    session = env->CallObjectMethod(gateway,method);
    session = env->NewGlobalRef(session);

    fatalException();
    qDebug() << "established session";

}

void Java::getBuffer() {
    jmethodID getBufferMethod = env->GetMethodID(sessionClass,"getBuffer","()Ljava/nio/ByteBuffer;");
    fatalException();
    jobject directBuffer = env->CallObjectMethod(session,getBufferMethod);
    fatalException();
    void * address = env->GetDirectBufferAddress(directBuffer);
    fatalException();
    int capacity = env->GetDirectBufferCapacity(directBuffer);
    fatalException();
    buffer = new DirectBuffer(address,capacity);
    fatalException();
    qDebug() << "established DirectBuffer()";

}

void Java::fatalException() {
    env = getJNIEnv();
    if(env->ExceptionOccurred() != NULL) {
        env->ExceptionDescribe();
        exit(-1);
    }
}

void Java::handleLine(const QString &input, int line) {

    if(lock.try_lock() == false) {
        qDebug("handleLine is locked!");
        exit(-1);
        lock.lock();
    }
    env = getJNIEnv();
    fatalException();

    buffer->clear();
    buffer->write(EVENT_LINE);
    buffer->write(line);
    buffer->write(input);

    env->CallVoidMethod(session,eventMethod);
    fatalException();

    int cmd = buffer->nextInt();

    while(cmd != CMD_END) {
        handleCommand(cmd);
    }

    lock.unlock();
}

void Java::handleCommand(int cmd) {

    switch(cmd) {
    case CMD_ECHO:      cmdEcho();      break;
    case CMD_SEND:      cmdSend();      break;
    default:
        qCritical() << "Unhandled command: " << cmd;
        exit(-1);
        return;
    }

    qDebug() << "command handled";
}

void Java::cmdEcho() {
    char *ch = buffer->nextString();
    QString qs(ch);
    host->console->echo(qs);
}

void Java::cmdSend() {
    char *ch = buffer->nextString();
    QString qs(ch);
    host->send(qs);
}

void Java::createVM() {

    if(jvm != NULL) {
        qDebug() << "create JVM called but already made";
        return;
    }

    options = new JavaVMOption[1];
    options[0].optionString = "-Djava.class.path=/home/austin/workspace/mudlet/bin";
    vm_args.version = JNI_VERSION_1_6;
    vm_args.nOptions = 1;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = false;
    /* load and initialize a Java VM, return a JNI interface
     * pointer in env */

    jint rv;
    rv = JNI_CreateJavaVM(&jvm, (void**)&env, (void*)&vm_args);
    qDebug() << "Java :: createJavaVM called with rv==" << rv;
    if(rv < 0) {
        exit(-1);
        return;
    }

    delete options;

}

void Java::eventInit() {

    eventMethod = env->GetMethodID(sessionClass,"event","()V");
    fatalException();

    buffer->clear();
    buffer->write(EVENT_INIT);
    buffer->write(hostname);
    env->CallVoidMethod(session,eventMethod);
    fatalException();

}
