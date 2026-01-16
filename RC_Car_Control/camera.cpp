#include "camera.h"

extern std::atomic<bool> running;

int openCamera() {

    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Erreur : Impossible d'ouvrir la caméra !" << std::endl;
        return -1;
    }

    // Crée une fenêtre nommée
    const std::string windowName = "Caméra Plein Écran";
    cv::namedWindow(windowName, cv::WINDOW_NORMAL); // Permet le redimensionnement
    cv::setWindowProperty(windowName, cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN); // Met en plein écran

    cv::Mat frame;
    while (running) {
        cap >> frame; // Capture une image
        if (frame.empty()) {
            std::cerr << "Erreur : Image vide capturée." << std::endl;
            break;
        }

        cv::imshow(windowName, frame); // Affiche l'image

        // Quitte avec la touche 'q' ou 'ESC'
        char key = (char)cv::waitKey(1);
        if (key == 27 || key == 'q') {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
}

void cameraThreadFunc() {
    openCamera();
}