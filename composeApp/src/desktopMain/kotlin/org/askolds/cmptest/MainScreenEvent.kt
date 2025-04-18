package org.askolds.cmptest

sealed class MainScreenEvent {
    data object DoStuff: MainScreenEvent()
}