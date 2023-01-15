// ASCII spectrogram example for complex inputs. This example demonstrates
// the functionality of the ASCII spectrogram. A sweeping complex sinusoid
// is generated and the resulting spectral periodogram is printed to the
// screen.

#include "util/ws_spectrogram.h"



wsSpectrogram::wsSpectrogram(int port) : WebSocketServer(port) {

    LOG_TEST_INFO("wsSpectrogram::wsSpectrogram() port {} ", port);

    terminated.store(false);
    stopping.store(false);

    m_isWsRunning.store(false);
    m_socketsOn.store(false);
}


wsSpectrogram::~wsSpectrogram() {

    WebSocketServer::~WebSocketServer();

    terminated.store(false);
    stopping.store(false);

    LOG_TEST_INFO("wsSpectrogram::~wsSpectrogram()");

}

void wsSpectrogram::threadMain() {
    terminated.store(false);
    stopping.store(false);
    try {
        run();
    }
    catch (...) {
        terminated.store(true);
        stopping.store(true);
        throw;
    }
  
    terminated.store(true);
    stopping.store(true);
}



void wsSpectrogram::run() {
    // stuff goes here


    m_isWsRunning.store(true);
//int ws_spectrogram(RadioThread* sdr, RadioThreadIQDataQueuePtr iqpipe_rx, std::string spd_file) {

    using namespace std::complex_literals;

    LOG_TEST_DEBUG("wsSpectrogram::run()");

    m_IQdataQueue = std::static_pointer_cast<RadioThreadIQDataQueue>(wsSpectrogram::getQueue());

    LOG_TEST_DEBUG("wsSpectrogram::run() m_IQdataQueue use_cout {}", m_IQdataQueue.use_count());

    // options
    unsigned int nfft        =  512;    // transform size

    unsigned int i;
    unsigned int n;

    while(!stopping)
    {

        //poll the websocket .. this would normally be done by WebSocketServer::run()
        WebSocketServer::wait(-1);  //hack ?? timeout = 0 does not work is blocking


        m_IQdataOut = std::make_shared<RadioThreadIQData>();

    //    LOG_TEST_DEBUG("wsSpectrogram::run() queue size {}", m_IQdataQueue->size());

        if(m_IQdataQueue->size() > 5) {

            m_IQdataQueue->pop(m_IQdataOut);

            int samplesCnt = m_IQdataOut->data.size();

            liquid_float_complex x[m_IQdataOut->data.size()];


            i = 0;
            for(auto sample: m_IQdataOut->data)
            {
                // x[i].real(sample.real());
                // x[i].imag(sample.imag());
                x[i] = sample;
                i++;
            }
            if(m_socketsOn) {
    
                std::lock_guard < std::mutex > lock(m_onSockets_mutex);           

                float sp_psd[nfft];

                // initialize objects
                spgramcf q = spgramcf_create_default(nfft);


                // write block of samples to the spectrogram object
                spgramcf_write(q, x, m_IQdataOut->data.size());

                // print result to screen
                spgramcf_get_psd(q, sp_psd);

                // Handle SDR FFT
                m_msgSOCKET.clear();
                m_msgSOCKET.str("");
                m_msgSOCKET << "{\"center\":[";
                m_msgSOCKET << m_rxFreq;
                m_msgSOCKET << "],";
                m_msgSOCKET << "\"span\":[";
                m_msgSOCKET << m_span;
                m_msgSOCKET << "],";
                // m_msgSOCKET << "\"txFreq\":[";
                // m_msgSOCKET << m_txFreq;
                // m_msgSOCKET << "],";
                m_msgSOCKET << "\"s\":[";

                int i = 0;

                while (i < nfft)
                {
                    m_msgSOCKET << std::to_string((int)sp_psd[i]);
                    if (i < nfft - 1)
                    {
                        m_msgSOCKET << ",";
                    }
                    i++;
                }
                m_msgSOCKET << "]}";
                // msgSOCKET.seekp(-1, std::ios_base::end);
                broadcast(m_msgSOCKET.str());

                spgramcf_destroy(q);
            }
        }
    }

    m_isWsRunning.store(false);
    LOG_TEST_DEBUG("wsSpectrogram::run() done");
}


void wsSpectrogram::terminate() {
    stopping.store(true);
}




void wsSpectrogram::setQueue(const ThreadIQDataQueueBasePtr& threadQueue) {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    LOG_TEST_DEBUG("wsSpectrogram::setQueue()");
    m_IQdataQueueBase = threadQueue;
}



ThreadIQDataQueueBasePtr wsSpectrogram::getQueue() {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    LOG_TEST_DEBUG("wsSpectrogram::getQueue() ");
    return m_IQdataQueueBase;
}


void wsSpectrogram::onConnect(int socketID) {
    std::lock_guard < std::mutex > lock(m_onSockets_mutex);
    LOG_TEST_INFO("wsSpectrogram::onConnect() socketID # {} ", socketID);

    m_socketsOn.store(true);
}

void wsSpectrogram::onMessage(int socketID, const string& data) {


    LOG_TEST_INFO("User click: {} ", data);

    std::string cmd;
    int par = 0;

    if (data.find_first_of(':') > 0)
    {
        cmd = data.substr(0, data.find_first_of(':'));
    }


    if (data.find_first_of(':') > 0)
    {
        par = stoi(data.substr(data.find_first_of(':') + 1));
    }


    LOG_TEST_INFO("cmd: {} par: {} ", cmd, par);




}

void wsSpectrogram::onDisconnect(int socketID) {
    std::lock_guard < std::mutex > lock(m_onSockets_mutex);
    LOG_TEST_INFO("wsSpectrogram::onDisconnect() socketID # {} ", socketID);

    m_socketsOn.store(false);

}

void wsSpectrogram::onError(int socketID, const string& message) {

    LOG_TEST_ERROR("wsSpectrogram::onError() socketID # {} - {} ", socketID, message);
}












