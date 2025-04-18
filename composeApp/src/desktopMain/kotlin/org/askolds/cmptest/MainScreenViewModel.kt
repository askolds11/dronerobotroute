package org.askolds.cmptest

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.launch
import org.askolds.cmptest.opencv.OpenCVStuff

class MainScreenViewModel: ViewModel() {
    val t = OpenCVStuff()

    init {
        viewModelScope.launch {
            try {
                print(t.Otsu1())
//                print(t.Otsu())
//                print(t.Otsu())
//                print(t.Otsu())
//                print(t.Otsu())
//                print(t.Otsu())
//                print(t.Otsu())
//                print(t.Otsu())
//                print(t.Otsu())
//                print(t.Otsu())
//                print(t.Otsu())
//                print(t.Otsu())
//                print(t.Otsu())
//                print(t.Otsu())
//                print(t.Otsu())
            } catch (ex: Exception) {
                println(ex.stackTrace)
            }

        }
    }

    fun onEvent(event: MainScreenEvent) {
        when (event) {
            MainScreenEvent.DoStuff -> TODO()
        }
    }
}